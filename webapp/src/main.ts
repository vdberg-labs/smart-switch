import { CONFIG_CHAR_UUID, EngineConfig, LIMITS, SERVICE_UUID, decode, encode, isValid } from "./protocol";

const connectBtn = document.getElementById("connect") as HTMLButtonElement;
const saveBtn = document.getElementById("save") as HTMLButtonElement;
const form = document.getElementById("form") as HTMLFormElement;
const statusEl = document.getElementById("status") as HTMLDivElement;

const numChannelsEl = document.getElementById("numChannels") as HTMLSelectElement;
const pollMsEl = document.getElementById("pollMs") as HTMLInputElement;
const longPressMsEl = document.getElementById("longPressMs") as HTMLInputElement;
const multiGapMsEl = document.getElementById("multiGapMs") as HTMLInputElement;
const channelEls = [0, 1].map((i) => ({
	maxTaps: document.getElementById(`ch${i}-maxTaps`) as HTMLInputElement,
	holdEnabled: document.getElementById(`ch${i}-holdEnabled`) as HTMLInputElement,
}));

let configChar: BluetoothRemoteGATTCharacteristic | undefined;

function setStatus(message: string, kind: "info" | "error" | "ok" = "info"): void {
	statusEl.textContent = message;
	statusEl.className = kind === "error" ? "error" : kind === "ok" ? "ok" : "";
}

function readForm(): EngineConfig {
	return {
		numChannels: Number(numChannelsEl.value),
		pollMs: Number(pollMsEl.value),
		longPressMs: Number(longPressMsEl.value),
		multiGapMs: Number(multiGapMsEl.value),
		channels: [
			{ maxTaps: Number(channelEls[0].maxTaps.value), holdEnabled: channelEls[0].holdEnabled.checked },
			{ maxTaps: Number(channelEls[1].maxTaps.value), holdEnabled: channelEls[1].holdEnabled.checked },
		],
	};
}

function writeForm(cfg: EngineConfig): void {
	numChannelsEl.value = String(cfg.numChannels);
	pollMsEl.value = String(cfg.pollMs);
	longPressMsEl.value = String(cfg.longPressMs);
	multiGapMsEl.value = String(cfg.multiGapMs);
	cfg.channels.forEach((ch, i) => {
		channelEls[i].maxTaps.value = String(ch.maxTaps);
		channelEls[i].holdEnabled.checked = ch.holdEnabled;
	});
}

async function connect(): Promise<void> {
	if (!navigator.bluetooth) {
		setStatus("Web Bluetooth is not available in this browser. Use Chrome or Edge.", "error");
		return;
	}

	try {
		setStatus("Requesting device...");
		const device = await navigator.bluetooth.requestDevice({
			filters: [{ services: [SERVICE_UUID] }],
			optionalServices: [SERVICE_UUID],
		});

		device.addEventListener("gattserverdisconnected", () => {
			configChar = undefined;
			form.classList.remove("enabled");
			saveBtn.disabled = true;
			setStatus("Disconnected. The device's config window may have closed.", "error");
		});

		setStatus("Connecting...");
		const server = await device.gatt?.connect();
		const service = await server?.getPrimaryService(SERVICE_UUID);
		configChar = await service?.getCharacteristic(CONFIG_CHAR_UUID);

		await readConfig();

		form.classList.add("enabled");
		saveBtn.disabled = false;
		setStatus(`Connected to ${device.name ?? "device"}.`, "ok");
	} catch (err) {
		setStatus(`Connect failed: ${(err as Error).message}`, "error");
	}
}

async function readConfig(): Promise<void> {
	if (!configChar) {
		return;
	}
	const value = await configChar.readValue();
	writeForm(decode(value));
}

async function save(): Promise<void> {
	if (!configChar) {
		return;
	}

	const cfg = readForm();
	if (!isValid(cfg)) {
		setStatus(
			`Out of range. Limits: maxTaps ${LIMITS.maxTaps.min}-${LIMITS.maxTaps.max}, ` +
				`poll ${LIMITS.pollMs.min}-${LIMITS.pollMs.max}ms, ` +
				`longPress ${LIMITS.longPressMs.min}-${LIMITS.longPressMs.max}ms, ` +
				`multiGap ${LIMITS.multiGapMs.min}-${LIMITS.multiGapMs.max}ms.`,
			"error",
		);
		return;
	}

	try {
		setStatus("Saving...");
		await configChar.writeValueWithResponse(encode(cfg) as BufferSource);
		await readConfig();
		setStatus("Saved.", "ok");
	} catch (err) {
		setStatus(`Save failed: ${(err as Error).message}`, "error");
	}
}

connectBtn.addEventListener("click", () => void connect());
saveBtn.addEventListener("click", () => void save());
form.addEventListener("submit", (e) => e.preventDefault());
