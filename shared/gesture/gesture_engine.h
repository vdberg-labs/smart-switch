#pragma once

#include "config_mode_trigger.h"
#include "network_removal_trigger.h"
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
	GestureEngine(ISwitchProtocol &sink, IConfigModeTrigger &configTrigger,
		      INetworkRemovalTrigger &networkRemovalTrigger);
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
		bool networkRemovalFired; /* 20s-hold network removal already fired for this press */
		uint8_t pressCount;
		int64_t pressStart;
		int64_t lastRelease;
	};

	/* Per-channel GPIO edge callback; carries a back-pointer since Channel lives in an array. */
	struct EdgeCallback {
		gpio_callback cb;
		GestureEngine *engine;
	};

	int EnableEdgeInterrupts();
	void DisableEdgeInterrupts();
	void Process(Channel &ch, int64_t now);
	int64_t NextDeadline(const Channel &ch) const;
	void EmitTap(Channel &ch, uint8_t taps);
	static void WorkHandler(k_work *work);
	static void ApplyWorkHandler(k_work *work);
	static void EdgeHandler(const device *port, gpio_callback *cb, gpio_port_pins_t pins);

	ISwitchProtocol &mSink;
	IConfigModeTrigger &mConfigTrigger;
	INetworkRemovalTrigger &mNetworkRemovalTrigger;

	uint8_t mNumActiveChannels;
	/* Debounce window: inputs are sampled once no edge has arrived for this long.
	 * Fed from EngineConfig::pollMs (name kept for wire/attribute compatibility). */
	uint32_t mDebounceMs;
	int64_t mLongPressMs;
	int64_t mMultiGapMs;

	mutable k_spinlock mConfigLock;
	EngineConfig mConfig; /* guarded by mConfigLock; authoritative copy for GetConfig()/ApplyConfig() */

	/* Work items wrapped with a back-pointer (like EdgeCallback): GestureEngine itself isn't
	 * standard-layout (reference members), so CONTAINER_OF on it trips -Winvalid-offsetof. */
	struct SampleWork {
		k_work_delayable work;
		GestureEngine *engine;
	};
	struct ApplyWork {
		k_work work;
		GestureEngine *engine;
	};

	SampleWork mWork;
	ApplyWork mApplyWork;

	static constexpr size_t kNumChannels = kMaxChannels;
	Channel mChannels[kNumChannels];
	EdgeCallback mEdgeCallbacks[kNumChannels];
};
}
