const IMAGE_MAGIC = 0x96f3b83d;
const VERSION_OFFSET = 20;
const HEADER_MIN_SIZE = 32;

/* Returns the version of a signed MCUboot image, or undefined if the file is not one. */
export function readImageVersion(image: Uint8Array): string | undefined {
	if (image.length < HEADER_MIN_SIZE) {
		return undefined;
	}

	const view = new DataView(image.buffer, image.byteOffset, image.byteLength);
	if (view.getUint32(0, true) !== IMAGE_MAGIC) {
		return undefined;
	}

	const major = view.getUint8(VERSION_OFFSET);
	const minor = view.getUint8(VERSION_OFFSET + 1);
	const revision = view.getUint16(VERSION_OFFSET + 2, true);
	return `${major}.${minor}.${revision}`;
}
