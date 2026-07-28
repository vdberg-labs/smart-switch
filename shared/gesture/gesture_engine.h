#pragma once

#include "config_mode_trigger.h"
#include "switch_protocol.h"

#include "../config/gesture_config.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>

#include <cstddef>
#include <cstdint>

namespace SmartSwitch
{

class GestureEngine {
public:
	GestureEngine(ISwitchProtocol &sink, IConfigModeTrigger &trigger);
	int Start();

	void ApplyConfig(const EngineConfig &cfg);

	/* Thread-safe snapshot of the currently active configuration (for the BLE read path). */
	EngineConfig GetConfig() const;

private:
	struct Channel {
		const gpio_dt_spec *spec;
		const char *name;
		uint8_t index; /* logical channel reported to the protocol (0 = SWITCH1) */
		ChannelConfig cfg;
		bool pressed;
		bool longFired;
		bool handled;         /* single tap already emitted on the press edge (fast path) */
		bool configModeFired; /* 10s-hold BLE-config trigger already fired for this press */
		uint8_t pressCount;
		int64_t pressStart;
		int64_t lastRelease;
	};

	void Process(Channel &ch, int64_t now);
	void EmitTap(Channel &ch, uint8_t taps);
	static void WorkHandler(k_work *work);
	static void ApplyWorkHandler(k_work *work);

	ISwitchProtocol &mSink;
	IConfigModeTrigger &mConfigTrigger;

	uint8_t mNumActiveChannels;
	uint32_t mPollMs;
	int64_t mLongPressMs;
	int64_t mMultiGapMs;

	mutable k_spinlock mConfigLock;
	EngineConfig mConfig; /* guarded by mConfigLock; authoritative copy for GetConfig()/ApplyConfig() */

	k_work_delayable mWork;
	k_work mApplyWork;

	static constexpr size_t kNumChannels = kMaxChannels;
	Channel mChannels[kNumChannels];
};
}
