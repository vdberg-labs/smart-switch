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
}

GestureEngine::GestureEngine(ISwitchProtocol &sink, IConfigModeTrigger &trigger)
	: mSink(sink),
	  mConfigTrigger(trigger),
	  mNumActiveChannels(EngineConfig::Defaults().numChannels),
	  mPollMs(EngineConfig::Defaults().pollMs),
	  mLongPressMs(EngineConfig::Defaults().longPressMs),
	  mMultiGapMs(EngineConfig::Defaults().multiGapMs),
	  mConfig(EngineConfig::Defaults()),
	  mChannels{
		  { &sSwitch0, "SWITCH1", 0, EngineConfig::Defaults().channels[0], false, false, false, false, 0, 0, 0 },
		  { &sSwitch1, "SWITCH2", 1, EngineConfig::Defaults().channels[1], false, false, false, false, 0, 0, 0 },
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

	k_work_init_delayable(&mWork, WorkHandler);
	k_work_init(&mApplyWork, ApplyWorkHandler);
	k_work_reschedule(&mWork, K_MSEC(mPollMs));
	LOG_INF("Gesture engine started: SWITCH1=P1.01/C9, SWITCH2=P2.04/K9");
	return 0;
}

void GestureEngine::ApplyConfig(const EngineConfig &cfg)
{
	k_spinlock_key_t key = k_spin_lock(&mConfigLock);
	mConfig = cfg;
	k_spin_unlock(&mConfigLock, key);
	k_work_submit(&mApplyWork);
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
	} else if (ch.cfg.maxTaps >= 2 && ch.pressCount > 0 &&
		   (now - ch.lastRelease) >= mMultiGapMs) {
		uint8_t n = ch.pressCount;
		ch.pressCount = 0;
		EmitTap(ch, n);
	}
}

void GestureEngine::WorkHandler(k_work *work)
{
	k_work_delayable *dwork = k_work_delayable_from_work(work);
	GestureEngine *self = CONTAINER_OF(dwork, GestureEngine, mWork);

	int64_t now = k_uptime_get();
	for (uint8_t i = 0; i < self->mNumActiveChannels; i++) {
		self->Process(self->mChannels[i], now);
	}
	k_work_reschedule(&self->mWork, K_MSEC(self->mPollMs));
}

void GestureEngine::ApplyWorkHandler(k_work *work)
{
	GestureEngine *self = CONTAINER_OF(work, GestureEngine, mApplyWork);

	EngineConfig cfg;
	{
		k_spinlock_key_t key = k_spin_lock(&self->mConfigLock);
		cfg = self->mConfig;
		k_spin_unlock(&self->mConfigLock, key);
	}

	self->mNumActiveChannels = cfg.numChannels;
	self->mPollMs = cfg.pollMs;
	self->mLongPressMs = cfg.longPressMs;
	self->mMultiGapMs = cfg.multiGapMs;

	int64_t now = k_uptime_get();
	for (uint8_t i = 0; i < kNumChannels; i++) {
		Channel &ch = self->mChannels[i];
		ch.cfg = cfg.channels[i];
		ch.longFired = false;
		ch.handled = false;
		ch.configModeFired = false;
		ch.pressCount = 0;
		if (ch.pressed) {
			ch.pressStart = now; /* restart the hold timer against the new threshold */
		}
	}

	LOG_INF("Gesture engine: config applied (channels=%u poll=%ums long=%ums gap=%ums)",
		self->mNumActiveChannels, self->mPollMs, static_cast<uint32_t>(self->mLongPressMs),
		static_cast<uint32_t>(self->mMultiGapMs));
}

} /* namespace SmartSwitch */
