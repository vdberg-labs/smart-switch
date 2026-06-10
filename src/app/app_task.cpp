/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "app/matter_init.h"
#include "app/task_executor.h"
#include "board/board.h"
#include "clusters/identify.h"

#include <setup_payload/OnboardingCodesUtil.h>

#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

#include <app/EventLogging.h>
#include <app-common/zap-generated/cluster-objects.h>
#include <app-common/zap-generated/attributes/Accessors.h>
#include <platform/CHIPDeviceLayer.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::DeviceLayer;

namespace
{
constexpr EndpointId kLightEndpointId = 1;

Nrf::Matter::IdentifyCluster sIdentifyCluster(kLightEndpointId);

/* --- Gesture engine for the two pulse-switch inputs (polled) ---
 * Samples each contact every kPollMs and classifies single / double / triple tap and hold, per
 * channel, matching the Matter Generic Switch (0x003B) event model (tap-count -> MultiPressComplete,
 * hold -> LongPress/LongRelease). sw0 = SWITCH1 (P1.01/C9), sw1 = SWITCH2 (P2.04/K9). Active-low.
 *
 * NOTE: polling (not edge interrupts) because on nRF54L15 port P2 has NO GPIOTE instance
 * (SoC DTS: gpio0->gpiote30, gpio1->gpiote20, gpio2->none), so SWITCH2 (P2.04) cannot use GPIO
 * edge interrupts (gpio_pin_interrupt_configure returns -ENOTSUP). A 5 ms poll is uniform across
 * both channels and negligible here (mains-powered, radio always on as an RxOn MED). */
const struct gpio_dt_spec sSwitch0 = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
const struct gpio_dt_spec sSwitch1 = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios);

constexpr int64_t kLongPressMs = 600; /* press held this long -> HOLD */
constexpr int64_t kMultiGapMs = 250;  /* max gap between taps in a multi-tap sequence */
constexpr uint32_t kPollMs = 5;       /* sampling interval (also coarse debounce) */

/* Per-channel gesture configuration.
 *   maxTaps:     1 = single only, 2 = up to double, 3 = up to triple.
 *   holdEnabled: detect a long press (HOLD / HOLD released).
 * Responsiveness: with maxTaps==1 AND holdEnabled==false a single tap fires immediately on the
 * press edge; with maxTaps==1 AND holdEnabled it fires on release; with maxTaps>=2 the engine
 * waits kMultiGapMs to count taps. Defaults are compile-time; can move to Kconfig/runtime. */
struct GestureConfig {
	uint8_t maxTaps;
	bool holdEnabled;
};

struct GestureChannel {
	const gpio_dt_spec *spec;
	const char *name;
	EndpointId endpoint; /* Matter Generic Switch endpoint this channel reports on */
	GestureConfig cfg;
	bool pressed;
	bool longFired;
	bool handled; /* single tap already emitted on the press edge (fast path) */
	uint8_t pressCount;
	int64_t pressStart;
	int64_t lastRelease;
};

/* Endpoints must match the Generic Switch endpoints created in the ZAP data model:
 * SWITCH1 -> endpoint 1, SWITCH2 -> endpoint 2. */
GestureChannel sChannels[] = {
	{ &sSwitch0, "SWITCH1", 1, { 3, true }, false, false, false, 0, 0, 0 },
	{ &sSwitch1, "SWITCH2", 2, { 3, true }, false, false, false, 0, 0, 0 },
};

k_work_delayable sGestureWork;

enum class SwEvt { InitialPress, ShortRelease, LongPress, LongRelease, MultiPressComplete };

/* Emit a Matter Generic Switch (0x003B) event on the given endpoint. Scheduled onto the Matter
 * thread (ScheduleLambda) because cluster/event APIs require the CHIP stack lock. Harmless no-op
 * until the Switch cluster is added to that endpoint via ZAP. */
void SwitchEmit(EndpointId ep, SwEvt evt, uint8_t count)
{
	chip::DeviceLayer::SystemLayer().ScheduleLambda([ep, evt, count]() {
		using namespace chip::app::Clusters;
		chip::EventNumber en = 0;
		switch (evt) {
		case SwEvt::InitialPress: {
			Switch::Attributes::CurrentPosition::Set(ep, 1);
			Switch::Events::InitialPress::Type e{ 1 };
			chip::app::LogEvent(e, ep, en);
			break;
		}
		case SwEvt::ShortRelease: {
			Switch::Attributes::CurrentPosition::Set(ep, 0);
			Switch::Events::ShortRelease::Type e{};
			e.previousPosition = 1;
			chip::app::LogEvent(e, ep, en);
			break;
		}
		case SwEvt::LongPress: {
			Switch::Events::LongPress::Type e{ 1 };
			chip::app::LogEvent(e, ep, en);
			break;
		}
		case SwEvt::LongRelease: {
			Switch::Attributes::CurrentPosition::Set(ep, 0);
			Switch::Events::LongRelease::Type e{};
			e.previousPosition = 1;
			chip::app::LogEvent(e, ep, en);
			break;
		}
		case SwEvt::MultiPressComplete: {
			Switch::Events::MultiPressComplete::Type e{};
			e.previousPosition = 0;
			e.totalNumberOfPressesCounted = count;
			chip::app::LogEvent(e, ep, en);
			break;
		}
		}
	});
}

void EmitTap(const GestureChannel &ch, uint8_t n)
{
	const char *label = (n == 1) ? "SINGLE TAP" : (n == 2) ? "DOUBLE TAP" : "TRIPLE TAP";
	LOG_INF("GESTURE %s: %s (count=%u)", ch.name, label, n);
	SwitchEmit(ch.endpoint, SwEvt::MultiPressComplete, n);
}

void GestureProcess(GestureChannel &ch, int64_t now)
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
			SwitchEmit(ch.endpoint, SwEvt::InitialPress, 0);
			if (ch.cfg.maxTaps == 1 && !ch.cfg.holdEnabled) {
				EmitTap(ch, 1); /* fast path: fire on press (also MultiPressComplete) */
				ch.pressCount = 0;
				ch.handled = true;
			}
		} else {
			/* release edge */
			if (ch.longFired) {
				LOG_INF("GESTURE %s: HOLD released", ch.name);
				SwitchEmit(ch.endpoint, SwEvt::LongRelease, 0);
				ch.pressCount = 0; /* a hold terminates the sequence */
			} else {
				SwitchEmit(ch.endpoint, SwEvt::ShortRelease, 0);
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
		if (ch.cfg.holdEnabled && !ch.handled && !ch.longFired && (now - ch.pressStart) >= kLongPressMs) {
			ch.longFired = true;
			LOG_INF("GESTURE %s: HOLD", ch.name);
			SwitchEmit(ch.endpoint, SwEvt::LongPress, 0);
		}
	} else if (ch.cfg.maxTaps >= 2 && ch.pressCount > 0 && (now - ch.lastRelease) >= kMultiGapMs) {
		uint8_t n = ch.pressCount;
		ch.pressCount = 0;
		EmitTap(ch, n);
	}
}

void GestureWorkHandler(k_work *)
{
	int64_t now = k_uptime_get();
	for (auto &ch : sChannels) {
		GestureProcess(ch, now);
	}
	k_work_reschedule(&sGestureWork, K_MSEC(kPollMs));
}
} /* namespace */

void AppTask::ButtonEventHandler(Nrf::ButtonState, Nrf::ButtonMask)
{
	/* No on-board buttons (CONFIG_DK_LIBRARY=n); the pulse-switch inputs are handled by the
	 * gesture engine. Kept as a no-op to satisfy the Board interface. */
}

CHIP_ERROR AppTask::Init()
{
	/* Initialize Matter stack */
	ReturnErrorOnFailure(Nrf::Matter::PrepareServer());

	if (!Nrf::GetBoard().Init(ButtonEventHandler)) {
		LOG_ERR("User interface initialization failed.");
		return CHIP_ERROR_INCORRECT_STATE;
	}

	/* Start the (polled) gesture engine on the two pulse-switch inputs. */
	if (gpio_is_ready_dt(&sSwitch0) && gpio_is_ready_dt(&sSwitch1)) {
		gpio_pin_configure_dt(&sSwitch0, GPIO_INPUT);
		gpio_pin_configure_dt(&sSwitch1, GPIO_INPUT);
		k_work_init_delayable(&sGestureWork, GestureWorkHandler);
		k_work_reschedule(&sGestureWork, K_MSEC(kPollMs));
		LOG_INF("Gesture engine started: SWITCH1=P1.01/C9, SWITCH2=P2.04/K9");
	} else {
		LOG_ERR("Switch GPIOs not ready");
	}

	/* Register Matter event handler that controls the connectivity status LED based on the captured Matter network
	 * state. */
	ReturnErrorOnFailure(Nrf::Matter::RegisterEventHandler(Nrf::Board::DefaultMatterEventHandler, 0));

	ReturnErrorOnFailure(sIdentifyCluster.Init());

	return Nrf::Matter::StartServer();
}

CHIP_ERROR AppTask::StartApp()
{
	ReturnErrorOnFailure(Init());

	while (true) {
		Nrf::DispatchNextTask();
	}

	return CHIP_NO_ERROR;
}
