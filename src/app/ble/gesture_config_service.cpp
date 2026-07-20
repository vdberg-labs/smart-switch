#include "gesture_config_service.h"

#include "../config/config_store.h"

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

namespace SmartSwitch
{
namespace
{
GestureEngine *sEngine;
const BleConfigMode *sConfigMode;

ssize_t ReadConfig(bt_conn *conn, const bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset)
{
	if (!sEngine) {
		/* The GATT service is statically registered at boot, before GestureConfigService::Init()
		 * runs from AppTask::Init(); this should be unreachable in practice (nothing is
		 * advertising this early), but fail safe rather than dereferencing null. */
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	uint8_t wire[ConfigStore::kWireSize];
	ConfigStore::Encode(sEngine->GetConfig(), wire);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, wire, sizeof(wire));
}

ssize_t WriteConfig(bt_conn *conn, const bt_gatt_attr *attr, const void *buf, uint16_t len, uint16_t offset,
		     uint8_t flags)
{
	if (!sEngine || !sConfigMode) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}
	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (len != ConfigStore::kWireSize) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}
	if (!sConfigMode->IsActive()) {
		LOG_WRN("Gesture config write rejected: BLE config mode is not active");
		return BT_GATT_ERR(BT_ATT_ERR_AUTHENTICATION);
	}

	EngineConfig cfg;
	if (!ConfigStore::Decode(static_cast<const uint8_t *>(buf), len, cfg)) {
		LOG_WRN("Gesture config write rejected: invalid config");
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	if (ConfigStore::Save(cfg) != CHIP_NO_ERROR) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}
	sEngine->ApplyConfig(cfg);

	return len;
}

} /* namespace */

BT_GATT_SERVICE_DEFINE(sGestureConfigService, BT_GATT_PRIMARY_SERVICE(BT_UUID_SMART_SWITCH_GESTURE_CONFIG_SERVICE),
			BT_GATT_CHARACTERISTIC(BT_UUID_SMART_SWITCH_GESTURE_CONFIG_CHAR,
						BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
						BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, ReadConfig, WriteConfig,
						nullptr), );

void GestureConfigService::Init(GestureEngine &engine, const BleConfigMode &configMode)
{
	sEngine = &engine;
	sConfigMode = &configMode;
}

} /* namespace SmartSwitch */
