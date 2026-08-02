#include "zigbee_ble_config_mode.h"

#include "shared/ble/gesture_config_service.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_LOG_DEFAULT_LEVEL);

namespace SmartSwitch
{
namespace
{
constexpr uint8_t kAdvFlags = BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR;

const bt_data sAdvertisingData[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, kAdvFlags),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, SMART_SWITCH_UUID_GESTURE_CONFIG_SERVICE_VAL),
};
const bt_data sScanResponseData[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

const bt_le_adv_param sAdvParam =
	BT_LE_ADV_PARAM_INIT(BT_LE_ADV_OPT_CONNECTABLE, BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, nullptr);

ZigbeeBleConfigMode *sInstance;

bt_conn_cb sConnCallbacks{};
} /* namespace */

int ZigbeeBleConfigMode::Init()
{
	sInstance = this;
	k_work_init_delayable(&mTimeoutWork, TimeoutHandler);
	sConnCallbacks.connected = &ZigbeeBleConfigMode::Connected;
	sConnCallbacks.disconnected = &ZigbeeBleConfigMode::Disconnected;
	bt_conn_cb_register(&sConnCallbacks);

	return bt_enable(nullptr);
}

bool ZigbeeBleConfigMode::StartAdvertising()
{
	int rc = bt_le_adv_start(&sAdvParam, sAdvertisingData, ARRAY_SIZE(sAdvertisingData), sScanResponseData,
				  ARRAY_SIZE(sScanResponseData));
	if (rc != 0) {
		LOG_ERR("BLE config mode: failed to start advertising: %d", rc);
		return false;
	}
	LOG_INF("BLE config mode: advertising");
	return true;
}

void ZigbeeBleConfigMode::OnConfigModeRequested()
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

void ZigbeeBleConfigMode::Stop()
{
	if (!mActive) {
		return;
	}
	mActive = false;
	mConn = nullptr;
	bt_le_adv_stop();
	k_work_cancel_delayable(&mTimeoutWork);

	LOG_INF("BLE config mode: closed");
}

void ZigbeeBleConfigMode::TimeoutHandler(k_work *)
{
	LOG_INF("BLE config mode: window timed out");
	sInstance->Stop();
}

void ZigbeeBleConfigMode::Connected(bt_conn *conn, uint8_t err)
{
	if (err != 0 || !sInstance->mActive) {
		return;
	}
	if (sInstance->mConn) {
		return;
	}
	sInstance->mConn = conn;

	k_work_reschedule(&sInstance->mTimeoutWork, K_MSEC(kConfigWindowMs));
}

void ZigbeeBleConfigMode::Disconnected(bt_conn *conn, uint8_t)
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
		LOG_INF("BLE config mode: client disconnected, not resuming advertising");
		return;
	}

	LOG_INF("BLE config mode: client disconnected, resuming advertising for the rest of the window");
	sInstance->StartAdvertising();
}

} /* namespace SmartSwitch */
