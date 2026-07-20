/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "switch_protocol.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#include <cstddef>
#include <cstdint>

namespace SmartSwitch
{

/* Per-channel gesture configuration.
 *   maxTaps:     1 = single only, 2 = up to double, 3 = up to triple, etc.
 *   holdEnabled: detect a long press (HOLD / HOLD released).
 * Responsiveness: with maxTaps==1 AND holdEnabled==false a single tap fires immediately on the press
 * edge; with maxTaps==1 AND holdEnabled it fires on release; with maxTaps>=2 the engine waits for the
 * multi-tap gap to count taps. */
struct GestureConfig {
	uint8_t maxTaps;
	bool holdEnabled;
};

class GestureEngine {
public:
	explicit GestureEngine(ISwitchProtocol &sink);
	int Start();

private:
	struct Channel {
		const gpio_dt_spec *spec;
		const char *name;
		uint8_t index; /* logical channel reported to the protocol (0 = SWITCH1) */
		GestureConfig cfg;
		bool pressed;
		bool longFired;
		bool handled; /* single tap already emitted on the press edge (fast path) */
		uint8_t pressCount;
		int64_t pressStart;
		int64_t lastRelease;
	};

	void Process(Channel &ch, int64_t now);
	void EmitTap(Channel &ch, uint8_t taps);
	static void WorkHandler(k_work *work);

	ISwitchProtocol &mSink;
	k_work_delayable mWork;

	static constexpr size_t kNumChannels = 2;
	Channel mChannels[kNumChannels];
};
}
