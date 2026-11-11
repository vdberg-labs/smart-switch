/* The small subset of CBOR that SMP uses. */

export type CborValue = number | string | boolean | Uint8Array | CborValue[] | CborMap;
export interface CborMap {
	[key: string]: CborValue;
}

const MAJOR_UINT = 0;
const MAJOR_NEGATIVE = 1;
const MAJOR_BYTES = 2;
const MAJOR_TEXT = 3;
const MAJOR_ARRAY = 4;
const MAJOR_MAP = 5;
const MAJOR_SIMPLE = 7;

const SIMPLE_FALSE = 20;
const SIMPLE_TRUE = 21;
const INDEFINITE = 31;
const BREAK = 0xff;

function encodeHead(major: number, value: number): number[] {
	const type = major << 5;
	if (value < 24) return [type | value];
	if (value < 0x100) return [type | 24, value];
	if (value < 0x10000) return [type | 25, value >> 8, value & 0xff];
	return [type | 26, (value >>> 24) & 0xff, (value >>> 16) & 0xff, (value >>> 8) & 0xff, value & 0xff];
}

function encodeInto(out: number[], value: CborValue): void {
	if (typeof value === "number") {
		if (!Number.isInteger(value) || value < 0) {
			throw new Error(`Unsupported CBOR number: ${value}`);
		}
		out.push(...encodeHead(MAJOR_UINT, value));
	} else if (typeof value === "boolean") {
		out.push((MAJOR_SIMPLE << 5) | (value ? SIMPLE_TRUE : SIMPLE_FALSE));
	} else if (typeof value === "string") {
		const text = new TextEncoder().encode(value);
		out.push(...encodeHead(MAJOR_TEXT, text.length));
		for (const byte of text) out.push(byte);
	} else if (value instanceof Uint8Array) {
		out.push(...encodeHead(MAJOR_BYTES, value.length));
		for (const byte of value) out.push(byte);
	} else if (Array.isArray(value)) {
		out.push(...encodeHead(MAJOR_ARRAY, value.length));
		for (const item of value) encodeInto(out, item);
	} else {
		const entries = Object.entries(value);
		out.push(...encodeHead(MAJOR_MAP, entries.length));
		for (const [key, item] of entries) {
			encodeInto(out, key);
			encodeInto(out, item);
		}
	}
}

export function encode(value: CborValue): Uint8Array {
	const out: number[] = [];
	encodeInto(out, value);
	return new Uint8Array(out);
}

class Decoder {
	private offset = 0;

	constructor(private readonly bytes: Uint8Array) {}

	private readArgument(info: number): number {
		if (info < 24) return info;
		const size = info === 24 ? 1 : info === 25 ? 2 : info === 26 ? 4 : 0;
		if (size === 0) {
			throw new Error(`Unsupported CBOR length: ${info}`);
		}
		let value = 0;
		for (let i = 0; i < size; i++) {
			value = value * 256 + this.bytes[this.offset++];
		}
		return value;
	}

	private readBytes(length: number): Uint8Array {
		const slice = this.bytes.subarray(this.offset, this.offset + length);
		this.offset += length;
		return slice;
	}

	private atBreak(): boolean {
		if (this.bytes[this.offset] !== BREAK) {
			return false;
		}
		this.offset++;
		return true;
	}

	private readArray(info: number): CborValue[] {
		const items: CborValue[] = [];
		if (info === INDEFINITE) {
			while (!this.atBreak()) items.push(this.read());
			return items;
		}
		for (let remaining = this.readArgument(info); remaining > 0; remaining--) {
			items.push(this.read());
		}
		return items;
	}

	private readMap(info: number): CborMap {
		const map: CborMap = {};
		if (info === INDEFINITE) {
			while (!this.atBreak()) {
				const key = String(this.read());
				map[key] = this.read();
			}
			return map;
		}
		for (let remaining = this.readArgument(info); remaining > 0; remaining--) {
			const key = String(this.read());
			map[key] = this.read();
		}
		return map;
	}

	read(): CborValue {
		const head = this.bytes[this.offset++];
		const major = head >> 5;
		const info = head & 0x1f;

		switch (major) {
			case MAJOR_UINT:
				return this.readArgument(info);
			case MAJOR_NEGATIVE:
				return -1 - this.readArgument(info);
			case MAJOR_BYTES:
				return this.readBytes(this.readArgument(info));
			case MAJOR_TEXT:
				return new TextDecoder().decode(this.readBytes(this.readArgument(info)));
			case MAJOR_ARRAY:
				return this.readArray(info);
			case MAJOR_MAP:
				return this.readMap(info);
			case MAJOR_SIMPLE:
				if (info === SIMPLE_FALSE) return false;
				if (info === SIMPLE_TRUE) return true;
		}
		throw new Error(`Unsupported CBOR item: 0x${head.toString(16)}`);
	}
}

export function decode(bytes: Uint8Array): CborValue {
	return new Decoder(bytes).read();
}
