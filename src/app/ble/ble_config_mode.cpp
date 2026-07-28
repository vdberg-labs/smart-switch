#include "ble_config_mode.h"

#include "../../../shared/ble/gesture_config_service.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace chip::DeviceLayer;

namespace SmartSwitch
{
namespace
{
/* Zephyr 4.x (this NCS version) deprecated BT_LE_ADV_OPT_CONNECTABLE/BT_LE_ADV_OPT_ONE_TIME in
 * favor of a single BT_LE_ADV_OPT_CONN, matching what Matter's own BLEManagerImpl uses here. */
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
	sConnCallbacks.disconnected = &BleConfigMode::Disconnected;
	return bt_conn_cb_register(&sConnCallbacks);
}

void BleConfigMode::OnConfigModeRequested()
{
	if (mActive) {
		/* Already open: just push the timeout back out. */
		k_work_reschedule(&mTimeoutWork, K_MSEC(kConfigWindowMs));
		return;
	}

	mAdvRequest.priority = 1; /* below Matter's own CHIP_DEVICE_BLE_ADVERTISING_PRIORITY (0) */
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
	BLEAdvertisingArbiter::CancelRequest(mAdvRequest);
	k_work_cancel_delayable(&mTimeoutWork);
	LOG_INF("BLE config mode: closed");
}

void BleConfigMode::TimeoutHandler(k_work *)
{
	LOG_INF("BLE config mode: window timed out");
	sInstance->Stop();
}

void BleConfigMode::Disconnected(bt_conn *, uint8_t)
{
	if (sInstance->mActive) {
		sInstance->Stop();
	}
}

} /* namespace SmartSwitch */
