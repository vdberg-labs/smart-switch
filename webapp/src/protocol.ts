/* Mirrors src/app/config/config_store.cpp (ConfigStore::Encode/Decode) and
 * src/app/config/gesture_config.h (EngineConfig, range constants) on the firmware side.
 * There is no shared schema/codegen yet — if the wire format or ranges change on the firmware
 * side, this file must be updated to match by hand. */

export const WIRE_SIZE = 12;
const FORMAT_VERSION = 1;

export const SERVICE_UUID = "5b854902-7ba7-4ce6-aa35-f734c828a00e";
export const CONFIG_CHAR_UUID = "db1147e1-d5c9-41a0-be76-cd36e7d85572";

export const LIMITS = {
	maxTaps: { min: 1, max: 5 },
	pollMs: { min: 1, max: 50 },
	longPressMs: { min: 200, max: 5000 },
	multiGapMs: { min: 50, max: 2000 },
	numChannels: { min: 1, max: 2 },
} as const;

export interface ChannelConfig {
	maxTaps: number;
	holdEnabled: boolean;
}

export interface EngineConfig {
	numChannels: number;
	pollMs: number;
	longPressMs: number;
	multiGapMs: number;
	channels: [ChannelConfig, ChannelConfig];
}

function inRange(value: number, range: { min: number; max: number }): boolean {
	return Number.isInteger(value) && value >= range.min && value <= range.max;
}

export function isValid(cfg: EngineConfig): boolean {
	return (
		inRange(cfg.numChannels, LIMITS.numChannels) &&
		inRange(cfg.pollMs, LIMITS.pollMs) &&
		inRange(cfg.longPressMs, LIMITS.longPressMs) &&
		inRange(cfg.multiGapMs, LIMITS.multiGapMs) &&
		cfg.channels.every((ch) => inRange(ch.maxTaps, LIMITS.maxTaps))
	);
}

export function encode(cfg: EngineConfig): Uint8Array {
	const buf = new Uint8Array(WIRE_SIZE);
	const view = new DataView(buf.buffer);
	buf[0] = FORMAT_VERSION;
	buf[1] = cfg.numChannels;
	view.setUint16(2, cfg.pollMs, true);
	view.setUint16(4, cfg.longPressMs, true);
	view.setUint16(6, cfg.multiGapMs, true);
	buf[8] = cfg.channels[0].maxTaps;
	buf[9] = cfg.channels[0].holdEnabled ? 1 : 0;
	buf[10] = cfg.channels[1].maxTaps;
	buf[11] = cfg.channels[1].holdEnabled ? 1 : 0;
	return buf;
}

export function decode(data: DataView): EngineConfig {
	if (data.byteLength !== WIRE_SIZE) {
		throw new Error(`Unexpected config length: ${data.byteLength} (expected ${WIRE_SIZE})`);
	}
	if (data.getUint8(0) !== FORMAT_VERSION) {
		throw new Error(`Unsupported config format version: ${data.getUint8(0)}`);
	}

	const cfg: EngineConfig = {
		numChannels: data.getUint8(1),
		pollMs: data.getUint16(2, true),
		longPressMs: data.getUint16(4, true),
		multiGapMs: data.getUint16(6, true),
		channels: [
			{ maxTaps: data.getUint8(8), holdEnabled: data.getUint8(9) !== 0 },
			{ maxTaps: data.getUint8(10), holdEnabled: data.getUint8(11) !== 0 },
		],
	};

	if (!isValid(cfg)) {
		throw new Error("Device returned an out-of-range config");
	}

	return cfg;
}
