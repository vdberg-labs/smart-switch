import "@awesome.me/webawesome/dist/styles/webawesome.css";
import "@awesome.me/webawesome/dist/components/button/button.js";
import "@awesome.me/webawesome/dist/components/number-input/number-input.js";
import "@awesome.me/webawesome/dist/components/select/select.js";
import "@awesome.me/webawesome/dist/components/option/option.js";
import "@awesome.me/webawesome/dist/components/switch/switch.js";
import "@awesome.me/webawesome/dist/components/card/card.js";
import "@awesome.me/webawesome/dist/components/callout/callout.js";

import { CONFIG_CHAR_UUID, SCHEMA_CHAR_UUID, SERVICE_UUID } from "./protocol";
import {
	ConfigField,
	ConfigSchema,
	ConfigValues,
	decodeValues,
	encodeValues,
	fieldValueKey,
	isValid,
	parseSchema,
} from "./schema";

const connectBtn = document.getElementById("connect") as HTMLElement & { loading: boolean };
const saveBtn = document.getElementById("save") as HTMLElement & { disabled: boolean };
const formHost = document.getElementById("form") as HTMLDivElement;
const statusEl = document.getElementById("status") as HTMLDivElement;

let configChar: BluetoothRemoteGATTCharacteristic | undefined;
let schema: ConfigSchema | undefined;

function setStatus(message: string, variant: "neutral" | "danger" | "success" = "neutral"): void {
	statusEl.innerHTML = "";
	if (!message) {
		return;
	}
	const callout = document.createElement("wa-callout");
	callout.setAttribute("variant", variant);
	callout.textContent = message;
	statusEl.appendChild(callout);
}

/** Builds the form DOM from the device's own schema — no hardcoded field names, so this never
 * needs updating when firmware's config fields change. */
function renderForm(s: ConfigSchema): void {
	formHost.innerHTML = "";

	const ungrouped = s.fields.filter((f) => f.group === undefined);
	if (ungrouped.length > 0) {
		formHost.appendChild(buildCard(undefined, ungrouped));
	}

	for (const group of s.groups) {
		const fields = s.fields.filter((f) => f.group === group.id && f.groupIndex === group.index);
		formHost.appendChild(buildCard(group.label, fields));
	}
}

function buildCard(title: string | undefined, fields: ConfigField[]): HTMLElement {
	const card = document.createElement("wa-card");
	if (title) {
		const header = document.createElement("div");
		header.slot = "header";
		header.textContent = title;
		card.appendChild(header);
	}

	for (const field of fields) {
		card.appendChild(buildFieldControl(field));
	}
	return card;
}

function buildFieldControl(field: ConfigField): HTMLElement {
	const key = fieldValueKey(field);

	if (field.control === "select" && field.options) {
		const select = document.createElement("wa-select");
		select.setAttribute("label", field.label);
		select.dataset.key = key;
		for (const option of field.options) {
			const opt = document.createElement("wa-option");
			opt.setAttribute("value", String(option));
			opt.textContent = String(option);
			select.appendChild(opt);
		}
		return select;
	}

	if (field.type === "bool") {
		const wrapper = document.createElement("div");
		wrapper.className = "field-row";
		const sw = document.createElement("wa-switch");
		sw.dataset.key = key;
		sw.textContent = field.label;
		wrapper.appendChild(sw);
		return wrapper;
	}

	const input = document.createElement("wa-number-input");
	input.setAttribute("label", field.label);
	if (field.min !== undefined) input.setAttribute("min", String(field.min));
	if (field.max !== undefined) input.setAttribute("max", String(field.max));
	input.dataset.key = key;
	return input;
}

function readForm(s: ConfigSchema): ConfigValues {
	const values: ConfigValues = {};
	for (const field of s.fields) {
		const key = fieldValueKey(field);
		const el = formHost.querySelector<HTMLElement & { value?: string; checked?: boolean }>(`[data-key="${key}"]`);
		if (!el) continue;
		values[key] = field.type === "bool" ? Boolean(el.checked) : Number(el.value);
	}
	return values;
}

function writeForm(s: ConfigSchema, values: ConfigValues): void {
	for (const field of s.fields) {
		const key = fieldValueKey(field);
		const el = formHost.querySelector<HTMLElement & { value?: string; checked?: boolean }>(`[data-key="${key}"]`);
		if (!el) continue;
		const value = values[key];
		if (field.type === "bool") {
			el.checked = Boolean(value);
		} else {
			el.value = String(value);
		}
	}
}

async function connect(): Promise<void> {
	if (!navigator.bluetooth) {
		setStatus("Web Bluetooth is not available in this browser. Use Chrome or Edge.", "danger");
		return;
	}

	try {
		connectBtn.loading = true;
		setStatus("Requesting device...");
		const device = await navigator.bluetooth.requestDevice({
			filters: [{ services: [SERVICE_UUID] }],
			optionalServices: [SERVICE_UUID],
		});

		device.addEventListener("gattserverdisconnected", () => {
			configChar = undefined;
			schema = undefined;
			formHost.classList.remove("enabled");
			saveBtn.disabled = true;
			setStatus("Disconnected. The device's config window may have closed.", "danger");
		});

		setStatus("Connecting...");
		const server = await device.gatt?.connect();
		const service = await server?.getPrimaryService(SERVICE_UUID);

		const schemaChar = await service?.getCharacteristic(SCHEMA_CHAR_UUID);
		const schemaValue = await schemaChar?.readValue();
		if (!schemaValue) {
			throw new Error("Could not read device config schema");
		}
		schema = parseSchema(new TextDecoder().decode(schemaValue));
		renderForm(schema);

		configChar = await service?.getCharacteristic(CONFIG_CHAR_UUID);
		await readConfig();

		formHost.classList.add("enabled");
		saveBtn.disabled = false;
		setStatus(`Connected to ${device.name ?? "device"}.`, "success");
	} catch (err) {
		setStatus(`Connect failed: ${(err as Error).message}`, "danger");
	} finally {
		connectBtn.loading = false;
	}
}

async function readConfig(): Promise<void> {
	if (!configChar || !schema) {
		return;
	}
	const value = await configChar.readValue();
	writeForm(schema, decodeValues(schema, value));
}

async function save(): Promise<void> {
	if (!configChar || !schema) {
		return;
	}

	const values = readForm(schema);
	if (!isValid(schema, values)) {
		setStatus("One or more values are out of range for this device.", "danger");
		return;
	}

	try {
		setStatus("Saving...");
		await configChar.writeValueWithResponse(encodeValues(schema, values) as BufferSource);
		await readConfig();
		setStatus("Saved.", "success");
	} catch (err) {
		setStatus(`Save failed: ${(err as Error).message}`, "danger");
	}
}

connectBtn.addEventListener("click", () => void connect());
saveBtn.addEventListener("click", () => void save());
