#include "gesture_config_service.h"

#include "../config/config_store.h"
#include "../diag/device_log.h"

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_LOG_DEFAULT_LEVEL);

namespace SmartSwitch
{
namespace
{
GestureEngine *sEngine;
IBleConfigModeState *sConfigMode;
IPersistentStore *sStore;
IFactoryResetTrigger *sFactoryReset;
ICommissioningInfoProvider *sCommissioningInfo;
INetworkRemovalTrigger *sNetworkRemoval;

constexpr uint8_t kFactoryResetMagic = 0xF5;
constexpr uint8_t kNetworkRemovalMagic = 0xF6;

constexpr char kConfigSchemaJson[] =
	R"json({"schemaVersion":1,"wireFormatVersion":1,"wireSize":12,)json"
	R"json("groups":[{"id":"channel","index":0,"label":"Channel 1 (SWITCH1)"},)json"
	R"json({"id":"channel","index":1,"label":"Channel 2 (SWITCH2)"}],)json"
	R"json("fields":[)json"
	R"json({"key":"numChannels","label":"Active channels","type":"u8","offset":1,"size":1,"min":1,"max":2,"control":"select","options":[1,2]},)json"
	R"json({"key":"pollMs","label":"Debounce (ms)","type":"u16","offset":2,"size":2,"min":1,"max":50},)json"
	R"json({"key":"longPressMs","label":"Long-press / hold threshold (ms)","type":"u16","offset":4,"size":2,"min":200,"max":5000},)json"
	R"json({"key":"multiGapMs","label":"Multi-tap gap (ms)","type":"u16","offset":6,"size":2,"min":50,"max":2000},)json"
	R"json({"key":"maxTaps","label":"Max taps","type":"u8","offset":8,"size":1,"min":1,"max":5,"group":"channel","groupIndex":0},)json"
	R"json({"key":"holdEnabled","label":"Hold enabled","type":"bool","offset":9,"size":1,"group":"channel","groupIndex":0},)json"
	R"json({"key":"maxTaps","label":"Max taps","type":"u8","offset":10,"size":1,"min":1,"max":5,"group":"channel","groupIndex":1},)json"
	R"json({"key":"holdEnabled","label":"Hold enabled","type":"bool","offset":11,"size":1,"group":"channel","groupIndex":1})json"
	R"json(]})json";

ssize_t ReadSchema(bt_conn *conn, const bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset)
{
	if (!sConfigMode) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}
	if (!sConfigMode->IsActive()) {
		LOG_WRN("Gesture config schema read rejected: BLE config mode is not active");
		return BT_GATT_ERR(BT_ATT_ERR_AUTHENTICATION);
	}
	return bt_gatt_attr_read(conn, attr, buf, len, offset, kConfigSchemaJson, sizeof(kConfigSchemaJson) - 1);
}

ssize_t ReadConfig(bt_conn *conn, const bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset)
{
	if (!sEngine) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	uint8_t wire[ConfigStore::kWireSize];
	ConfigStore::Encode(sEngine->GetConfig(), wire);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, wire, sizeof(wire));
}

ssize_t WriteConfig(bt_conn *conn, const bt_gatt_attr *attr, const void *buf, uint16_t len, uint16_t offset,
		     uint8_t flags)
{
	if (!sEngine || !sConfigMode || !sStore) {
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

	if (!ConfigStore::Save(*sStore, cfg)) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}
	sEngine->ApplyConfig(cfg);

	return len;
}

ssize_t WriteFactoryReset(bt_conn *conn, const bt_gatt_attr *attr, const void *buf, uint16_t len, uint16_t offset,
			   uint8_t flags)
{
	if (!sConfigMode || !sFactoryReset) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}
	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (len != 1) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}
	if (!sConfigMode->IsActive()) {
		LOG_WRN("Factory reset rejected: BLE config mode is not active");
		return BT_GATT_ERR(BT_ATT_ERR_AUTHENTICATION);
	}
	if (static_cast<const uint8_t *>(buf)[0] != kFactoryResetMagic) {
		LOG_WRN("Factory reset rejected: wrong confirmation value");
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	LOG_INF("Factory reset requested over BLE");
	sFactoryReset->OnFactoryResetRequested();

	return len;
}

ssize_t WriteRemoveNetwork(bt_conn *conn, const bt_gatt_attr *attr, const void *buf, uint16_t len, uint16_t offset,
			    uint8_t flags)
{
	if (!sConfigMode || !sNetworkRemoval) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}
	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (len != 1) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}
	if (!sConfigMode->IsActive()) {
		LOG_WRN("Remove-from-network rejected: BLE config mode is not active");
		return BT_GATT_ERR(BT_ATT_ERR_AUTHENTICATION);
	}
	if (static_cast<const uint8_t *>(buf)[0] != kNetworkRemovalMagic) {
		LOG_WRN("Remove-from-network rejected: wrong confirmation value");
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	LOG_INF("Remove-from-network requested over BLE");
	sNetworkRemoval->OnNetworkRemovalRequested();

	return len;
}

ssize_t ReadCommissioningInfo(bt_conn *conn, const bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset)
{
	if (!sConfigMode || !sCommissioningInfo) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}
	if (!sConfigMode->IsActive()) {
		LOG_WRN("Commissioning info read rejected: BLE config mode is not active");
		return BT_GATT_ERR(BT_ATT_ERR_AUTHENTICATION);
	}

	char json[192];
	size_t jsonLen = 0;
	if (!sCommissioningInfo->GetCommissioningInfoJson(json, sizeof(json), jsonLen)) {
		LOG_ERR("Failed to build commissioning info JSON");
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	sConfigMode->SuppressResumeOnDisconnect();

	return bt_gatt_attr_read(conn, attr, buf, len, offset, json, jsonLen);
}

ssize_t ReadLog(bt_conn *conn, const bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset)
{
	if (!sConfigMode) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}
	if (!sConfigMode->IsActive()) {
		LOG_WRN("Device log read rejected: BLE config mode is not active");
		return BT_GATT_ERR(BT_ATT_ERR_AUTHENTICATION);
	}

	size_t snapshotLen = 0;
	const char *snapshot = DeviceLog::Snapshot(snapshotLen);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, snapshot, snapshotLen);
}

} /* namespace */

BT_GATT_SERVICE_DEFINE(sGestureConfigService, BT_GATT_PRIMARY_SERVICE(BT_UUID_SMART_SWITCH_GESTURE_CONFIG_SERVICE),
			BT_GATT_CHARACTERISTIC(BT_UUID_SMART_SWITCH_GESTURE_CONFIG_CHAR,
						BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
						BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, ReadConfig, WriteConfig,
						nullptr),
			BT_GATT_CHARACTERISTIC(BT_UUID_SMART_SWITCH_GESTURE_CONFIG_SCHEMA_CHAR, BT_GATT_CHRC_READ,
						BT_GATT_PERM_READ, ReadSchema, nullptr, nullptr),
			BT_GATT_CHARACTERISTIC(BT_UUID_SMART_SWITCH_GESTURE_FACTORY_RESET_CHAR, BT_GATT_CHRC_WRITE,
						BT_GATT_PERM_WRITE, nullptr, WriteFactoryReset, nullptr),
			BT_GATT_CHARACTERISTIC(BT_UUID_SMART_SWITCH_GESTURE_COMMISSIONING_CHAR, BT_GATT_CHRC_READ,
						BT_GATT_PERM_READ, ReadCommissioningInfo, nullptr, nullptr),
			BT_GATT_CHARACTERISTIC(BT_UUID_SMART_SWITCH_GESTURE_LOG_CHAR, BT_GATT_CHRC_READ,
						BT_GATT_PERM_READ, ReadLog, nullptr, nullptr),
			BT_GATT_CHARACTERISTIC(BT_UUID_SMART_SWITCH_GESTURE_REMOVE_NETWORK_CHAR, BT_GATT_CHRC_WRITE,
						BT_GATT_PERM_WRITE, nullptr, WriteRemoveNetwork, nullptr), );

void GestureConfigService::Init(GestureEngine &engine, IBleConfigModeState &configMode, IPersistentStore &store,
				 IFactoryResetTrigger &factoryReset, ICommissioningInfoProvider &commissioningInfo,
				 INetworkRemovalTrigger &networkRemoval)
{
	sEngine = &engine;
	sConfigMode = &configMode;
	sStore = &store;
	sFactoryReset = &factoryReset;
	sCommissioningInfo = &commissioningInfo;
	sNetworkRemoval = &networkRemoval;
}

} /* namespace SmartSwitch */
