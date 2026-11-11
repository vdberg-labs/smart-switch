/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "ble/ble_config_mode.h"
#include "config/matter_persistent_store.h"
#include "dfu/image_confirmation.h"
#include "matter/matter_commissioning_info.h"
#include "matter/matter_fabric_delegate.h"
#include "matter/matter_factory_reset.h"
#include "matter/matter_gesture_config_cluster.h"
#include "matter/matter_network_removal.h"
#include "matter/matter_switch_protocol.h"

#include "shared/ble/gesture_config_service.h"
#include "shared/config/config_store.h"
#include "shared/gesture/gesture_engine.h"

#include "app/matter_init.h"
#include "app/task_executor.h"
#include "board/board.h"
#include "clusters/identify.h"

#include <app/server/Server.h>
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

SmartSwitch::MatterSwitchProtocol sSwitchProtocol;
SmartSwitch::BleConfigMode sBleConfigMode;
SmartSwitch::MatterPersistentStore sPersistentStore;
SmartSwitch::MatterFactoryReset sFactoryReset;
SmartSwitch::MatterCommissioningInfo sCommissioningInfo;
SmartSwitch::MatterNetworkRemoval sNetworkRemoval;
SmartSwitch::MatterFabricDelegate sFabricDelegate;
SmartSwitch::GestureEngine sGestureEngine(sSwitchProtocol, sBleConfigMode, sNetworkRemoval);
} /* namespace */

SmartSwitch::GestureEngine &AppTask::GetGestureEngine()
{
	return sGestureEngine;
}

SmartSwitch::IPersistentStore &AppTask::GetPersistentStore()
{
	return sPersistentStore;
}

void AppTask::ButtonEventHandler(Nrf::ButtonState, Nrf::ButtonMask)
{
	/* No on-board buttons (CONFIG_DK_LIBRARY=n); the pulse-switch inputs are handled by the
	 * gesture engine. Kept as a no-op to satisfy the Board interface. */
}

CHIP_ERROR AppTask::Init()
{
	SmartSwitch::ImageConfirmation::ArmRevertTimer();

	ReturnErrorOnFailure(Nrf::Matter::PrepareServer());

	if (!Nrf::GetBoard().Init(ButtonEventHandler)) {
		LOG_ERR("User interface initialization failed.");
		return CHIP_ERROR_INCORRECT_STATE;
	}

	sGestureEngine.Start();
	sGestureEngine.ApplyConfig(SmartSwitch::ConfigStore::Load(sPersistentStore));
	SmartSwitch::RefreshGestureConfigAttributes();
	sBleConfigMode.Init();
	SmartSwitch::GestureConfigService::Init(sGestureEngine, sBleConfigMode, sPersistentStore, sFactoryReset,
						 sCommissioningInfo, sNetworkRemoval);
	chip::Server::GetInstance().GetFabricTable().AddFabricDelegate(&sFabricDelegate);

	ReturnErrorOnFailure(Nrf::Matter::RegisterEventHandler(Nrf::Board::DefaultMatterEventHandler, 0));
	ReturnErrorOnFailure(SmartSwitch::ImageConfirmation::ConfirmOnceServerIsHealthy());

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
