import { CborMap, decode, encode } from "./cbor";

export const SMP_SERVICE_UUID = "8d53dc1d-1db7-4cd3-868b-8a527460aa84";
const SMP_CHAR_UUID = "da2e7828-fbce-4e01-ae9e-261174997c48";

const OP_READ = 0;
const OP_WRITE = 2;

const GROUP_OS = 0;
const GROUP_IMAGE = 1;
const ID_OS_RESET = 5;
const ID_IMAGE_STATE = 0;
const ID_IMAGE_UPLOAD = 1;

const HEADER_SIZE = 8;
const SECONDARY_SLOT = 1;
const RC_ACCESS_DENIED = 11;

/* Fits the ATT MTU of every Web Bluetooth platform; the device reassembles fragments. */
const WRITE_FRAGMENT_SIZE = 180;
/* Leaves room for the SMP header and CBOR framing in the device's 512-byte buffer. */
const UPLOAD_CHUNK_SIZE = 384;
const REQUEST_TIMEOUT_MS = 15000;

export interface ImageSlot {
	slot: number;
	version: string;
	hash: Uint8Array;
	active: boolean;
	confirmed: boolean;
	pending: boolean;
}

interface PendingRequest {
	sequence: number;
	resolve: (body: CborMap) => void;
	reject: (error: Error) => void;
}

function describeError(rc: number): string {
	return rc === RC_ACCESS_DENIED
		? "The device refused the update. Hold a switch for 10 seconds and reconnect."
		: `The device reported error ${rc}.`;
}

export class SmpClient {
	private sequence = 0;
	private pending: PendingRequest | undefined;
	private received = new Uint8Array(0);

	private constructor(private readonly characteristic: BluetoothRemoteGATTCharacteristic) {
		characteristic.addEventListener("characteristicvaluechanged", () => this.onNotification());
	}

	static async open(server: BluetoothRemoteGATTServer): Promise<SmpClient> {
		const service = await server.getPrimaryService(SMP_SERVICE_UUID);
		const characteristic = await service.getCharacteristic(SMP_CHAR_UUID);
		await characteristic.startNotifications();
		return new SmpClient(characteristic);
	}

	async listImages(): Promise<ImageSlot[]> {
		const response = await this.request(OP_READ, GROUP_IMAGE, ID_IMAGE_STATE, {});
		return response.images as unknown as ImageSlot[];
	}

	async upload(image: Uint8Array, onProgress: (sent: number, total: number) => void): Promise<void> {
		const sha = new Uint8Array(await crypto.subtle.digest("SHA-256", image as BufferSource));

		let offset = 0;
		while (offset < image.length) {
			const body: CborMap = { data: image.subarray(offset, offset + UPLOAD_CHUNK_SIZE), off: offset };
			if (offset === 0) {
				body.len = image.length;
				body.sha = sha;
			}
			const response = await this.request(OP_WRITE, GROUP_IMAGE, ID_IMAGE_UPLOAD, body);
			offset = response.off as number;
			onProgress(offset, image.length);
		}
	}

	/* Marks the uploaded image to be swapped in on the next boot. */
	async scheduleUploadedImage(): Promise<void> {
		const images = await this.listImages();
		const uploaded = images.find((image) => image.slot === SECONDARY_SLOT);
		if (!uploaded) {
			throw new Error("The device does not report the uploaded image.");
		}
		await this.request(OP_WRITE, GROUP_IMAGE, ID_IMAGE_STATE, { hash: uploaded.hash, confirm: false });
	}

	async reset(): Promise<void> {
		await this.request(OP_WRITE, GROUP_OS, ID_OS_RESET, {});
	}

	private request(op: number, group: number, id: number, body: CborMap): Promise<CborMap> {
		const sequence = this.sequence;
		this.sequence = (this.sequence + 1) & 0xff;

		const payload = encode(body);
		const message = new Uint8Array(HEADER_SIZE + payload.length);
		new DataView(message.buffer).setUint16(2, payload.length);
		new DataView(message.buffer).setUint16(4, group);
		message[0] = op;
		message[6] = sequence;
		message[7] = id;
		message.set(payload, HEADER_SIZE);

		return new Promise<CborMap>((resolve, reject) => {
			const timeout = setTimeout(() => {
				this.pending = undefined;
				reject(new Error("The device did not respond."));
			}, REQUEST_TIMEOUT_MS);

			this.received = new Uint8Array(0);
			this.pending = {
				sequence,
				resolve: (response) => {
					clearTimeout(timeout);
					resolve(response);
				},
				reject: (error) => {
					clearTimeout(timeout);
					reject(error);
				},
			};
			this.send(message).catch((error: Error) => this.pending?.reject(error));
		});
	}

	private async send(message: Uint8Array): Promise<void> {
		for (let offset = 0; offset < message.length; offset += WRITE_FRAGMENT_SIZE) {
			const fragment = message.subarray(offset, offset + WRITE_FRAGMENT_SIZE);
			await this.characteristic.writeValueWithoutResponse(fragment as BufferSource);
		}
	}

	private onNotification(): void {
		const value = this.characteristic.value;
		if (!value) {
			return;
		}

		const combined = new Uint8Array(this.received.length + value.byteLength);
		combined.set(this.received);
		combined.set(new Uint8Array(value.buffer, value.byteOffset, value.byteLength), this.received.length);
		this.received = combined;

		if (this.received.length < HEADER_SIZE) {
			return;
		}
		const payloadLength = new DataView(this.received.buffer).getUint16(2);
		if (this.received.length < HEADER_SIZE + payloadLength) {
			return;
		}
		this.complete(this.received);
	}

	private complete(message: Uint8Array): void {
		const pending = this.pending;
		if (!pending || message[6] !== pending.sequence) {
			return;
		}
		this.pending = undefined;

		try {
			const response = decode(message.subarray(HEADER_SIZE)) as CborMap;
			const rc = (response.rc ?? (response.err as CborMap | undefined)?.rc ?? 0) as number;
			if (rc !== 0) {
				pending.reject(new Error(describeError(rc)));
				return;
			}
			pending.resolve(response);
		} catch (error) {
			pending.reject(error as Error);
		}
	}
}
