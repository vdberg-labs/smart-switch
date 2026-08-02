#pragma once

#include "shared/gesture/ble_config_state.h"
#include "shared/gesture/config_mode_trigger.h"

#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>

namespace SmartSwitch
{

class ZigbeeBleConfigMode : public IConfigModeTrigger, public IBleConfigModeState {
public:
	int Init();

	void OnConfigModeRequested() override;

	bool IsActive() const override { return mActive; }
	void SuppressResumeOnDisconnect() override { mSuppressResumeOnDisconnect = true; }

private:
	static constexpr uint32_t kConfigWindowMs = 5 * 60 * 1000; /* 5 minutes */

	bool StartAdvertising();
	void Stop();
	static void TimeoutHandler(k_work *work);
	static void Connected(bt_conn *conn, uint8_t err);
	static void Disconnected(bt_conn *conn, uint8_t reason);

	k_work_delayable mTimeoutWork;
	bool mActive = false;
	bt_conn *mConn = nullptr;
	bool mSuppressResumeOnDisconnect = false;
};

}
