#include "ble_config_mode.h"

#include "shared/ble/gesture_config_service.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace chip::DeviceLayer;

namespace SmartSwitch
{
namespace
{
constexpr uint32_t kAdvOptions = BT_LE_ADV_OPT_CONN;
constexpr uint8_t kAdvFlags = BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR;

const bt_data sAdvertisingData[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, kAdvFlags),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, SMART_SWITCH_UUID_GESTURE_CONFIG_SERVICE_VAL),
};
const bt_data sScanResponseData[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

BleConfigMode *sInstance;

bt_conn_cb sConnCallbacks{};
} /* namespace */

int BleConfigMode::Init()
{
	sInstance = this;
	k_work_init_delayable(&mTimeoutWork, TimeoutHandler);
	sConnCallbacks.connected = &BleConfigMode::Connected;
	sConnCallbacks.disconnected = &BleConfigMode::Disconnected;
	return bt_conn_cb_register(&sConnCallbacks);
}

bool BleConfigMode::StartAdvertising()
{
	mAdvRequest.priority = 1;
	mAdvRequest.options = kAdvOptions;
	mAdvRequest.minInterval = BT_GAP_ADV_FAST_INT_MIN_2;
	mAdvRequest.maxInterval = BT_GAP_ADV_FAST_INT_MAX_2;
	mAdvRequest.advertisingData = chip::Span<const bt_data>(sAdvertisingData);
	mAdvRequest.scanResponseData = chip::Span<const bt_data>(sScanResponseData);
	mAdvRequest.onStarted = [](int rc) {
		if (rc == 0) {
			LOG_INF("BLE config mode: advertising");
		} else {
			LOG_ERR("BLE config mode: failed to start advertising: %d", rc);
		}
	};

	CHIP_ERROR err = BLEAdvertisingArbiter::InsertRequest(mAdvRequest);
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("BLE config mode: advertising request failed");
		return false;
	}
	return true;
}

void BleConfigMode::OnConfigModeRequested()
{
	if (mActive && mConn) {
		k_work_reschedule(&mTimeoutWork, K_MSEC(kConfigWindowMs));
		return;
	}

	mSuppressResumeOnDisconnect = false;
	if (!StartAdvertising()) {
		return;
	}

	mActive = true;
	k_work_reschedule(&mTimeoutWork, K_MSEC(kConfigWindowMs));
}

void BleConfigMode::Stop()
{
	if (!mActive) {
		return;
	}
	mActive = false;
	mConn = nullptr;
	BLEAdvertisingArbiter::CancelRequest(mAdvRequest);
	k_work_cancel_delayable(&mTimeoutWork);

	LOG_INF("BLE config mode: closed");
}

void BleConfigMode::TimeoutHandler(k_work *)
{
	LOG_INF("BLE config mode: window timed out");
	sInstance->Stop();
}

void BleConfigMode::Connected(bt_conn *conn, uint8_t err)
{
	if (err != 0 || !sInstance->mActive) {
		return;
	}
	if (sInstance->mConn) {
		return;
	}
	sInstance->mConn = conn;

	k_work_reschedule(&sInstance->mTimeoutWork, K_MSEC(kConfigWindowMs));

	BLEAdvertisingArbiter::CancelRequest(sInstance->mAdvRequest);
}

void BleConfigMode::Disconnected(bt_conn *conn, uint8_t)
{
	if (conn != sInstance->mConn) {
		return;
	}
	sInstance->mConn = nullptr;

	if (!sInstance->mActive) {
		return;
	}

	if (sInstance->mSuppressResumeOnDisconnect) {
		sInstance->mSuppressResumeOnDisconnect = false;
		LOG_INF("BLE config mode: client disconnected for commissioning handoff, not resuming advertising");
		return;
	}

	LOG_INF("BLE config mode: client disconnected, resuming advertising for the rest of the window");
	sInstance->StartAdvertising();
}

} /* namespace SmartSwitch */
