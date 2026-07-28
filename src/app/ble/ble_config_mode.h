#pragma once

#include "../../../shared/gesture/ble_config_state.h"
#include "../../../shared/gesture/config_mode_trigger.h"

#include <platform/Zephyr/BLEAdvertisingArbiter.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>

namespace SmartSwitch
{

/* Controls the on-demand BLE advertising window used to reach the gesture config GATT service.
 * Advertising only runs while a config-mode window is open (entered via a 10s hold on either
 * switch channel, see GestureEngine); the rest of the time the custom service exists in the GATT
 * database but is unreachable since the device isn't connectable for it. Advertising is
 * coordinated through Matter's existing BLEAdvertisingArbiter so it can't collide with Matter's
 * own CHIPoBLE commissioning advertising (which always wins arbitration). */
class BleConfigMode : public IConfigModeTrigger, public IBleConfigModeState {
public:
	/* Registers the connection callbacks used to end the window early on disconnect. Call once
	 * from AppTask::Init(), after Nrf::Matter::PrepareServer() (which initializes the BLE
	 * advertising arbiter). */
	int Init();

	void OnConfigModeRequested() override;

	bool IsActive() const override { return mActive; }

private:
	static constexpr uint32_t kConfigWindowMs = 5 * 60 * 1000; /* 5 minutes */

	void Stop();
	static void TimeoutHandler(k_work *work);
	static void Disconnected(bt_conn *conn, uint8_t reason);

	chip::DeviceLayer::BLEAdvertisingArbiter::Request mAdvRequest{};
	k_work_delayable mTimeoutWork;
	bool mActive = false;
};

}
