import "@awesome.me/webawesome/dist/styles/webawesome.css";
import "@awesome.me/webawesome/dist/components/button/button.js";
import "@awesome.me/webawesome/dist/components/number-input/number-input.js";
import "@awesome.me/webawesome/dist/components/select/select.js";
import "@awesome.me/webawesome/dist/components/option/option.js";
import "@awesome.me/webawesome/dist/components/switch/switch.js";
import "@awesome.me/webawesome/dist/components/card/card.js";
import "@awesome.me/webawesome/dist/components/callout/callout.js";

import QRCode from "qrcode";

import {
	COMMISSIONING_CHAR_UUID,
	CONFIG_CHAR_UUID,
	FACTORY_RESET_CHAR_UUID,
	FACTORY_RESET_MAGIC,
	LOG_CHAR_UUID,
	REMOVE_NETWORK_CHAR_UUID,
	REMOVE_NETWORK_MAGIC,
	SCHEMA_CHAR_UUID,
	SERVICE_UUID,
} from "./protocol";
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

interface CommissioningInfo {
	protocol: string;
	qrCode: string;
	manualPairingCode: string;
	windowOpen: boolean;
	commissioned: boolean;
}

const connectBtn = document.getElementById("connect") as HTMLElement & { loading: boolean };
const saveBtn = document.getElementById("save") as HTMLElement & { disabled: boolean };
const factoryResetBtn = document.getElementById("factory-reset") as HTMLElement & { disabled: boolean };
const readLogBtn = document.getElementById("read-log") as HTMLElement & { disabled: boolean; loading: boolean };
const logOutputEl = document.getElementById("log-output") as HTMLPreElement;
const formHost = document.getElementById("form") as HTMLDivElement;
const statusEl = document.getElementById("status") as HTMLDivElement;

const tabsEl = document.getElementById("tabs") as HTMLDivElement;
const tabConfigBtn = document.getElementById("tab-config") as HTMLElement & { variant: string };
const tabNetworkBtn = document.getElementById("tab-network") as HTMLElement & { variant: string };
const tabLogsBtn = document.getElementById("tab-logs") as HTMLElement & { variant: string };
const tabResetBtn = document.getElementById("tab-reset") as HTMLElement & { variant: string };
const sectionConfig = document.getElementById("section-config") as HTMLElement;
const sectionNetwork = document.getElementById("section-network") as HTMLElement;
const sectionLogs = document.getElementById("section-logs") as HTMLElement;
const sectionReset = document.getElementById("section-reset") as HTMLElement;
const commissionLoadingEl = document.getElementById("commission-loading") as HTMLParagraphElement;
const commissionReadyEl = document.getElementById("commission-ready") as HTMLDivElement;
const commissionBlockedEl = document.getElementById("commission-blocked") as HTMLDivElement;
const commissionStatusDotEl = document.getElementById("commission-status-dot") as HTMLSpanElement;
const commissionStatusTextEl = document.getElementById("commission-status-text") as HTMLSpanElement;
const commissionBlockedDescEl = document.getElementById("commission-blocked-desc") as HTMLParagraphElement;
const networkInfoRowsEl = document.getElementById("network-info-rows") as HTMLDivElement;
const qrCanvas = document.getElementById("qr-canvas") as HTMLCanvasElement;
const manualCodeEl = document.getElementById("manual-code") as HTMLDivElement;
const qrTextEl = document.getElementById("qr-text") as HTMLDivElement;
const disconnectToCommissionBtn = document.getElementById("disconnect-to-commission") as HTMLElement;
const removeNetworkBtn = document.getElementById("remove-network") as HTMLElement & { disabled: boolean };

let configChar: BluetoothRemoteGATTCharacteristic | undefined;
let factoryResetChar: BluetoothRemoteGATTCharacteristic | undefined;
let logChar: BluetoothRemoteGATTCharacteristic | undefined;
let commissioningChar: BluetoothRemoteGATTCharacteristic | undefined;
let removeNetworkChar: BluetoothRemoteGATTCharacteristic | undefined;
let schema: ConfigSchema | undefined;
let currentDevice: BluetoothDevice | undefined;

/* Set right before we deliberately disconnect (either for a commissioning handoff, or after
 * remove-network/factory-reset, both of which reboot the device), so the "gattserverdisconnected"
 * handler doesn't stomp our own status message with a generic "Disconnected" warning. */
let disconnectingIntentionally = false;

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

function renderNetworkInfoRows(raw: unknown): void {
	networkInfoRowsEl.replaceChildren();
	if (typeof raw !== "object" || raw === null || !("joined" in raw)) {
		return;
	}

	const info = raw as Record<string, unknown>;
	const rows: Array<[string, string]> = [
		["Joined", info.joined ? "Yes" : "No"],
		["Role", String(info.role ?? "?")],
	];
	if (info.joined) {
		rows.push(
			["PAN ID", `0x${Number(info.panId).toString(16)}`],
			["Extended PAN ID", String(info.extendedPanId ?? "?")],
			["Channel", String(info.channel ?? "?")],
			["Short address", `0x${Number(info.shortAddress).toString(16)}`],
			["IEEE address", String(info.ieeeAddress ?? "?")],
		);
	}

	for (const [label, value] of rows) {
		const row = document.createElement("div");
		row.className = "code-row";
		const labelEl = document.createElement("div");
		labelEl.className = "label";
		labelEl.textContent = label;
		const valueEl = document.createElement("div");
		valueEl.className = "value";
		valueEl.textContent = value;
		row.append(labelEl, valueEl);
		networkInfoRowsEl.append(row);
	}
}

function parseCommissioningInfo(json: string): CommissioningInfo {
	const parsed: unknown = JSON.parse(json);
	if (
		typeof parsed !== "object" ||
		parsed === null ||
		typeof (parsed as CommissioningInfo).protocol !== "string" ||
		typeof (parsed as CommissioningInfo).qrCode !== "string" ||
		typeof (parsed as CommissioningInfo).manualPairingCode !== "string" ||
		typeof (parsed as CommissioningInfo).windowOpen !== "boolean" ||
		typeof (parsed as CommissioningInfo).commissioned !== "boolean"
	) {
		throw new Error("Malformed commissioning info from device");
	}
	return parsed as CommissioningInfo;
}

type Tab = "config" | "network" | "logs" | "reset";

function showTab(tab: Tab): void {
	sectionConfig.hidden = tab !== "config";
	sectionNetwork.hidden = tab !== "network";
	sectionLogs.hidden = tab !== "logs";
	sectionReset.hidden = tab !== "reset";
	tabConfigBtn.variant = tab === "config" ? "brand" : "neutral";
	tabNetworkBtn.variant = tab === "network" ? "brand" : "neutral";
	tabLogsBtn.variant = tab === "logs" ? "brand" : "neutral";
	tabResetBtn.variant = tab === "reset" ? "brand" : "neutral";

	if (tab === "network") {
		void loadCommissionTab();
	}
}

async function loadCommissionTab(): Promise<void> {
	commissionLoadingEl.style.display = "";
	commissionReadyEl.classList.remove("enabled");
	commissionBlockedEl.classList.remove("enabled");

	if (!commissioningChar) {
		return;
	}

	try {
		const value = await commissioningChar.readValue();
		const text = new TextDecoder().decode(value);
		const raw: unknown = JSON.parse(text);
		const protocol = typeof raw === "object" && raw !== null ? (raw as { protocol?: unknown }).protocol : undefined;

		commissionLoadingEl.style.display = "none";

		if (protocol !== "matter") {
			commissionStatusDotEl.className = "status-dot unsupported";
			commissionStatusTextEl.textContent = `Network commissioning info not supported for this device's protocol (${String(protocol ?? "unknown")})`;
			commissionBlockedDescEl.textContent =
				"This device doesn't expose a QR/manual pairing code over BLE for its protocol, but you can still remove it from its current network below.";
			renderNetworkInfoRows(raw);
			commissionBlockedEl.classList.add("enabled");
			removeNetworkBtn.disabled = !removeNetworkChar;
			return;
		}

		networkInfoRowsEl.replaceChildren();
		const info = parseCommissioningInfo(text);

		if (info.commissioned) {
			commissionStatusDotEl.className = "status-dot commissioned";
			commissionStatusTextEl.textContent = "Commissioned to a Matter network";
			commissionBlockedDescEl.textContent =
				"To commission this device onto a different controller, remove it from its current " +
				"network first, or use Factory reset in the Reset tab for a full reset. Either will reboot " +
				"the device; reconnect afterward to see its commissioning code here.";
			commissionBlockedEl.classList.add("enabled");
			removeNetworkBtn.disabled = !removeNetworkChar;
		} else {
			await QRCode.toCanvas(qrCanvas, info.qrCode, { margin: 1 });
			manualCodeEl.textContent = info.manualPairingCode;
			qrTextEl.textContent = info.qrCode;
			commissionReadyEl.classList.add("enabled");
		}
	} catch (err) {
		commissionLoadingEl.style.display = "none";
		setStatus(`Could not read commissioning info: ${(err as Error).message}`, "danger");
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
		currentDevice = device;

		device.addEventListener("gattserverdisconnected", () => {
			configChar = undefined;
			factoryResetChar = undefined;
			logChar = undefined;
			commissioningChar = undefined;
			removeNetworkChar = undefined;
			schema = undefined;
			tabsEl.classList.remove("enabled");
			saveBtn.disabled = true;
			factoryResetBtn.disabled = true;
			readLogBtn.disabled = true;
			removeNetworkBtn.disabled = true;
			logOutputEl.classList.remove("enabled");
			logOutputEl.textContent = "";
			commissionReadyEl.classList.remove("enabled");
			commissionBlockedEl.classList.remove("enabled");
			networkInfoRowsEl.replaceChildren();
			showTab("config");

			if (disconnectingIntentionally) {
				disconnectingIntentionally = false;
				return;
			}
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

		factoryResetChar = await service?.getCharacteristic(FACTORY_RESET_CHAR_UUID);
		logChar = await service?.getCharacteristic(LOG_CHAR_UUID);
		commissioningChar = await service?.getCharacteristic(COMMISSIONING_CHAR_UUID);
		removeNetworkChar = await service?.getCharacteristic(REMOVE_NETWORK_CHAR_UUID);

		formHost.classList.add("enabled");
		tabsEl.classList.add("enabled");
		saveBtn.disabled = false;
		factoryResetBtn.disabled = false;
		readLogBtn.disabled = false;
		showTab("config");
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

async function factoryReset(): Promise<void> {
	if (!factoryResetChar) {
		return;
	}

	const confirmed = confirm(
		"Factory reset this device?\n\n" +
			"It will leave its current network immediately, reset gesture settings to " +
			"defaults, and will need to be re-commissioned before it can be used again. " +
			"This page will disconnect. This cannot be undone.",
	);
	if (!confirmed) {
		return;
	}

	try {
		setStatus("Requesting factory reset...");
		await factoryResetChar.writeValueWithResponse(new Uint8Array([FACTORY_RESET_MAGIC]) as BufferSource);
		setStatus("Factory reset requested. The device is resetting and this page will disconnect.", "success");
	} catch (err) {
		setStatus(`Factory reset failed: ${(err as Error).message}`, "danger");
	}
}

async function removeNetwork(): Promise<void> {
	if (!removeNetworkChar) {
		return;
	}

	const confirmed = confirm(
		"Remove this device from its current network?\n\n" +
			"It will leave its current network and reboot, ready to be commissioned onto a different " +
			"controller. Unlike factory reset, gesture settings are kept. This page will disconnect.",
	);
	if (!confirmed) {
		return;
	}

	try {
		setStatus("Requesting removal from network...");
		await removeNetworkChar.writeValueWithResponse(new Uint8Array([REMOVE_NETWORK_MAGIC]) as BufferSource);
		setStatus("Removal requested. The device is rebooting and this page will disconnect.", "success");
	} catch (err) {
		setStatus(`Remove from network failed: ${(err as Error).message}`, "danger");
	}
}

function disconnectToCommission(): void {
	if (!currentDevice?.gatt?.connected) {
		return;
	}
	disconnectingIntentionally = true;
	currentDevice.gatt.disconnect();
	setStatus("Disconnected. Scan the code above (or enter it manually) with your controller now.", "success");
}

async function readLog(): Promise<void> {
	if (!logChar) {
		return;
	}

	try {
		readLogBtn.loading = true;
		const value = await logChar.readValue();
		const text = new TextDecoder().decode(value);
		logOutputEl.textContent = text || "(empty)";
		logOutputEl.classList.add("enabled");
	} catch (err) {
		setStatus(`Read log failed: ${(err as Error).message}`, "danger");
	} finally {
		readLogBtn.loading = false;
	}
}

connectBtn.addEventListener("click", () => void connect());
saveBtn.addEventListener("click", () => void save());
factoryResetBtn.addEventListener("click", () => void factoryReset());
readLogBtn.addEventListener("click", () => void readLog());
removeNetworkBtn.addEventListener("click", () => void removeNetwork());
disconnectToCommissionBtn.addEventListener("click", disconnectToCommission);
tabConfigBtn.addEventListener("click", () => showTab("config"));
tabNetworkBtn.addEventListener("click", () => showTab("network"));
tabLogsBtn.addEventListener("click", () => showTab("logs"));
tabResetBtn.addEventListener("click", () => showTab("reset"));
