/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "gesture/gesture_engine.h"
#include "matter/matter_switch_protocol.h"

#include "app/matter_init.h"
#include "app/task_executor.h"
#include "board/board.h"
#include "clusters/identify.h"

#include <setup_payload/OnboardingCodesUtil.h>

#include <zephyr/logging/log.h>

#include <platform/CHIPDeviceLayer.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::DeviceLayer;

namespace
{
constexpr EndpointId kLightEndpointId = 1;

Nrf::Matter::IdentifyCluster sIdentifyCluster(kLightEndpointId);

/* The gesture engine (protocol-agnostic) reports gestures to a communication backend through the
 * ISwitchProtocol interface. Today that backend is Matter (Generic Switch); a Zigbee backend slots in
 * by swapping only this one object — the engine is unchanged. */
SmartSwitch::MatterSwitchProtocol sSwitchProtocol;
SmartSwitch::GestureEngine sGestureEngine(sSwitchProtocol);
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

	sGestureEngine.Start();

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
