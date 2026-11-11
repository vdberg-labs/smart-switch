#include "gesture_engine.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_LOG_DEFAULT_LEVEL);

namespace SmartSwitch
{
namespace
{
const gpio_dt_spec sSwitch0 = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
const gpio_dt_spec sSwitch1 = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios);

constexpr int64_t kConfigModeHoldMs = 10000;
constexpr int64_t kNetworkRemovalHoldMs = 20000;
}

GestureEngine::GestureEngine(ISwitchProtocol &sink, IConfigModeTrigger &configTrigger,
			     INetworkRemovalTrigger &networkRemovalTrigger)
	: mSink(sink),
	  mConfigTrigger(configTrigger),
	  mNetworkRemovalTrigger(networkRemovalTrigger),
	  mNumActiveChannels(EngineConfig::Defaults().numChannels),
	  mDebounceMs(EngineConfig::Defaults().pollMs),
	  mLongPressMs(EngineConfig::Defaults().longPressMs),
	  mMultiGapMs(EngineConfig::Defaults().multiGapMs),
	  mConfig(EngineConfig::Defaults()),
	  mChannels{
		  { &sSwitch0, "SWITCH1", 0, EngineConfig::Defaults().channels[0], false, false, false, false, false, 0, 0, 0 },
		  { &sSwitch1, "SWITCH2", 1, EngineConfig::Defaults().channels[1], false, false, false, false, false, 0, 0, 0 },
	  }
{
}

int GestureEngine::Start()
{
	if (!gpio_is_ready_dt(&sSwitch0) || !gpio_is_ready_dt(&sSwitch1)) {
		LOG_ERR("Switch GPIOs not ready");
		return -ENODEV;
	}

	gpio_pin_configure_dt(&sSwitch0, GPIO_INPUT);
	gpio_pin_configure_dt(&sSwitch1, GPIO_INPUT);

	mWork.engine = this;
	mApplyWork.engine = this;
	k_work_init_delayable(&mWork.work, WorkHandler);
	k_work_init(&mApplyWork.work, ApplyWorkHandler);

	/* Both switch pins must be GPIOTE-capable (rev2+: both on port 1). There is deliberately
	 * no polling fallback - rev1 (SWITCH1 on P2.04, port 2 has no GPIOTE) is not supported. */
	int err = EnableEdgeInterrupts();
	if (err != 0) {
		LOG_ERR("Switch edge interrupts unavailable (%d) - switch inputs disabled", err);
		return err;
	}

	/* Sample once right away, so a switch already held at boot is seen. */
	k_work_reschedule(&mWork.work, K_NO_WAIT);
	LOG_INF("Gesture engine started: SWITCH1=%s pin %u, SWITCH2=%s pin %u", sSwitch0.port->name,
		sSwitch0.pin, sSwitch1.port->name, sSwitch1.pin);
	return 0;
}

int GestureEngine::EnableEdgeInterrupts()
{
	for (uint8_t i = 0; i < kNumChannels; i++) {
		const gpio_dt_spec *spec = mChannels[i].spec;
		mEdgeCallbacks[i].engine = this;
		gpio_init_callback(&mEdgeCallbacks[i].cb, EdgeHandler, BIT(spec->pin));

		int err = gpio_add_callback(spec->port, &mEdgeCallbacks[i].cb);
		if (err == 0) {
			err = gpio_pin_interrupt_configure_dt(spec, GPIO_INT_EDGE_BOTH);
		}
		if (err != 0) {
			DisableEdgeInterrupts();
			return err;
		}
	}
	return 0;
}

void GestureEngine::DisableEdgeInterrupts()
{
	for (uint8_t i = 0; i < kNumChannels; i++) {
		const gpio_dt_spec *spec = mChannels[i].spec;
		gpio_pin_interrupt_configure_dt(spec, GPIO_INT_DISABLE);
		gpio_remove_callback(spec->port, &mEdgeCallbacks[i].cb); /* -EINVAL if never added: fine */
	}
}

void GestureEngine::EdgeHandler(const device *, gpio_callback *cb, gpio_port_pins_t)
{
	/* ISR context: just (re)arm the debounce window. Every further bounce edge pushes it out
	 * again, so the inputs are sampled only once the line has been quiet for mDebounceMs. The
	 * final level is always sampled, so no edge can be lost - only sub-window glitches. */
	EdgeCallback *edge = CONTAINER_OF(cb, EdgeCallback, cb);
	GestureEngine *self = edge->engine;
	k_work_reschedule(&self->mWork.work, K_MSEC(self->mDebounceMs));
}

void GestureEngine::ApplyConfig(const EngineConfig &cfg)
{
	k_spinlock_key_t key = k_spin_lock(&mConfigLock);
	mConfig = cfg;
	k_spin_unlock(&mConfigLock, key);
	k_work_submit(&mApplyWork.work);
}

EngineConfig GestureEngine::GetConfig() const
{
	k_spinlock_key_t key = k_spin_lock(&mConfigLock);
	EngineConfig cfg = mConfig;
	k_spin_unlock(&mConfigLock, key);
	return cfg;
}

void GestureEngine::EmitTap(Channel &ch, uint8_t taps)
{
	const char *label = (taps == 1) ? "SINGLE TAP" : (taps == 2) ? "DOUBLE TAP" : "TRIPLE TAP";
	LOG_INF("GESTURE %s: %s (count=%u)", ch.name, label, taps);
	mSink.OnGesture(ch.index, Gesture::Tap, taps);
}

void GestureEngine::Process(Channel &ch, int64_t now)
{
	int level = gpio_pin_get_dt(ch.spec); /* logical: 1 = pressed, <0 = error */
	if (level < 0) {
		return;
	}
	bool nowPressed = (level > 0);

	if (nowPressed != ch.pressed) {
		ch.pressed = nowPressed;
		if (nowPressed) {
			/* press edge */
			ch.pressStart = now;
			ch.longFired = false;
			if (ch.pressCount < ch.cfg.maxTaps) {
				ch.pressCount++;
			}
			mSink.OnGesture(ch.index, Gesture::InitialPress, 0);
			if (ch.cfg.maxTaps == 1 && !ch.cfg.holdEnabled) {
				EmitTap(ch, 1); /* fast path: fire on press */
				ch.pressCount = 0;
				ch.handled = true;
			}
		} else {
			/* release edge */
			ch.configModeFired = false;
			ch.networkRemovalFired = false;
			if (ch.longFired) {
				LOG_INF("GESTURE %s: HOLD released", ch.name);
				mSink.OnGesture(ch.index, Gesture::LongRelease, 0);
				ch.pressCount = 0; /* a hold terminates the sequence */
			} else {
				mSink.OnGesture(ch.index, Gesture::ShortRelease, 0);
				if (ch.handled) {
					ch.handled = false;
				} else if (ch.cfg.maxTaps == 1) {
					EmitTap(ch, 1); /* single+hold config: fire on release */
					ch.pressCount = 0;
				}
			}
			ch.lastRelease = now;
		}
		return;
	}

	/* no edge: detect hold while pressed, or multi-tap completion while released */
	if (ch.pressed) {
		if (ch.cfg.holdEnabled && !ch.handled && !ch.longFired &&
		    (now - ch.pressStart) >= mLongPressMs) {
			ch.longFired = true;
			LOG_INF("GESTURE %s: HOLD", ch.name);
			mSink.OnGesture(ch.index, Gesture::LongPress, 0);
		}
		/* Independent of maxTaps/holdEnabled: a sufficiently long hold always opens BLE
		 * config mode, since it's a maintenance action rather than a configured gesture. */
		if (!ch.configModeFired && (now - ch.pressStart) >= kConfigModeHoldMs) {
			ch.configModeFired = true;
			LOG_INF("%s: BLE config mode requested (10s hold)", ch.name);
			mConfigTrigger.OnConfigModeRequested();
		}
		if (!ch.networkRemovalFired && (now - ch.pressStart) >= kNetworkRemovalHoldMs) {
			ch.networkRemovalFired = true;
			LOG_WRN("%s: leaving the network (20s hold)", ch.name);
			mNetworkRemovalTrigger.OnNetworkRemovalRequested();
		}
	} else if (ch.cfg.maxTaps >= 2 && ch.pressCount > 0 &&
		   (now - ch.lastRelease) >= mMultiGapMs) {
		uint8_t n = ch.pressCount;
		ch.pressCount = 0;
		EmitTap(ch, n);
	}
}

int64_t GestureEngine::NextDeadline(const Channel &ch) const
{
	int64_t next = INT64_MAX;
	if (ch.pressed) {
		if (ch.cfg.holdEnabled && !ch.handled && !ch.longFired) {
			next = MIN(next, ch.pressStart + mLongPressMs);
		}
		if (!ch.configModeFired) {
			next = MIN(next, ch.pressStart + kConfigModeHoldMs);
		}
		if (!ch.networkRemovalFired) {
			next = MIN(next, ch.pressStart + kNetworkRemovalHoldMs);
		}
	} else if (ch.cfg.maxTaps >= 2 && ch.pressCount > 0) {
		next = MIN(next, ch.lastRelease + mMultiGapMs);
	}
	return next;
}

void GestureEngine::WorkHandler(k_work *work)
{
	k_work_delayable *dwork = k_work_delayable_from_work(work);
	GestureEngine *self = CONTAINER_OF(dwork, SampleWork, work)->engine;

	int64_t now = k_uptime_get();
	for (uint8_t i = 0; i < self->mNumActiveChannels; i++) {
		self->Process(self->mChannels[i], now);
	}

	/* Wake only for the earliest pending timed event (hold, 10s config hold, 20s network removal,
	 * multi-tap gap); with nothing pending, stay idle until the next edge. k_work_schedule
	 * (not reschedule) on purpose: if an edge arrived while this handler ran, its debounce
	 * run is already queued and will recompute deadlines - don't push it back. */
	int64_t next = INT64_MAX;
	for (uint8_t i = 0; i < self->mNumActiveChannels; i++) {
		next = MIN(next, self->NextDeadline(self->mChannels[i]));
	}
	if (next != INT64_MAX) {
		int64_t delay = next - k_uptime_get();
		k_work_schedule(&self->mWork.work, K_MSEC(delay > 0 ? delay : 0));
	}
}

void GestureEngine::ApplyWorkHandler(k_work *work)
{
	GestureEngine *self = CONTAINER_OF(work, ApplyWork, work)->engine;

	EngineConfig cfg;
	{
		k_spinlock_key_t key = k_spin_lock(&self->mConfigLock);
		cfg = self->mConfig;
		k_spin_unlock(&self->mConfigLock, key);
	}

	self->mNumActiveChannels = cfg.numChannels;
	self->mDebounceMs = cfg.pollMs; /* config field keeps its wire name */
	self->mLongPressMs = cfg.longPressMs;
	self->mMultiGapMs = cfg.multiGapMs;

	int64_t now = k_uptime_get();
	for (uint8_t i = 0; i < kNumChannels; i++) {
		Channel &ch = self->mChannels[i];
		ch.cfg = cfg.channels[i];
		ch.longFired = false;
		ch.handled = false;
		ch.configModeFired = false;
		ch.networkRemovalFired = false;
		ch.pressCount = 0;
		if (ch.pressed) {
			ch.pressStart = now; /* restart the hold timer against the new threshold */
		}
	}

	/* There's no free-running loop: kick one run so deadlines are recomputed against the
	 * new thresholds (e.g. a channel held down right now). */
	k_work_reschedule(&self->mWork.work, K_NO_WAIT);

	LOG_INF("Gesture engine: config applied (channels=%u debounce=%ums long=%ums gap=%ums)",
		self->mNumActiveChannels, self->mDebounceMs, static_cast<uint32_t>(self->mLongPressMs),
		static_cast<uint32_t>(self->mMultiGapMs));
}

} /* namespace SmartSwitch */
