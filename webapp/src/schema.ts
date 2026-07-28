export interface ConfigGroup {
	id: string;
	index: number;
	label: string;
}

export interface ConfigField {
	key: string;
	label: string;
	type: "u8" | "u16" | "bool";
	offset: number;
	size: number;
	min?: number;
	max?: number;
	group?: string;
	groupIndex?: number;
	control?: "select";
	options?: number[];
}

export interface ConfigSchema {
	schemaVersion: number;
	wireFormatVersion: number;
	wireSize: number;
	groups: ConfigGroup[];
	fields: ConfigField[];
}

export type ConfigValues = Record<string, number | boolean>;

export function fieldValueKey(f: ConfigField): string {
	return f.group !== undefined ? `${f.group}.${f.groupIndex}.${f.key}` : f.key;
}

export function parseSchema(json: string): ConfigSchema {
	const schema = JSON.parse(json) as ConfigSchema;
	if (typeof schema.wireSize !== "number" || !Array.isArray(schema.fields)) {
		throw new Error("Malformed config schema");
	}
	return schema;
}

export function decodeValues(schema: ConfigSchema, data: DataView): ConfigValues {
	if (data.byteLength !== schema.wireSize) {
		throw new Error(`Unexpected config length: ${data.byteLength} (expected ${schema.wireSize})`);
	}
	if (data.getUint8(0) !== schema.wireFormatVersion) {
		throw new Error(`Unsupported config format version: ${data.getUint8(0)}`);
	}

	const values: ConfigValues = {};
	for (const field of schema.fields) {
		const key = fieldValueKey(field);
		switch (field.type) {
			case "bool":
				values[key] = data.getUint8(field.offset) !== 0;
				break;
			case "u8":
				values[key] = data.getUint8(field.offset);
				break;
			case "u16":
				values[key] = data.getUint16(field.offset, true);
				break;
		}
	}
	return values;
}

export function encodeValues(schema: ConfigSchema, values: ConfigValues): Uint8Array {
	const buf = new Uint8Array(schema.wireSize);
	const view = new DataView(buf.buffer);
	buf[0] = schema.wireFormatVersion;

	for (const field of schema.fields) {
		const key = fieldValueKey(field);
		const raw = values[key];
		switch (field.type) {
			case "bool":
				buf[field.offset] = raw ? 1 : 0;
				break;
			case "u8":
				buf[field.offset] = Number(raw);
				break;
			case "u16":
				view.setUint16(field.offset, Number(raw), true);
				break;
		}
	}
	return buf;
}

export function isValid(schema: ConfigSchema, values: ConfigValues): boolean {
	return schema.fields.every((field) => {
		const value = values[fieldValueKey(field)];
		if (field.type === "bool") {
			return typeof value === "boolean";
		}
		if (typeof value !== "number" || !Number.isInteger(value)) {
			return false;
		}
		if (field.min !== undefined && value < field.min) {
			return false;
		}
		if (field.max !== undefined && value > field.max) {
			return false;
		}
		if (field.options !== undefined && !field.options.includes(value)) {
			return false;
		}
		return true;
	});
}
