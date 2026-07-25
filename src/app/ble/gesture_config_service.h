#pragma once

#include "ble_config_mode.h"
#include "../gesture/gesture_engine.h"

#include <zephyr/bluetooth/uuid.h>

namespace SmartSwitch
{

/* 5b854902-7ba7-4ce6-aa35-f734c828a00e */
#define SMART_SWITCH_UUID_GESTURE_CONFIG_SERVICE_VAL \
	BT_UUID_128_ENCODE(0x5b854902, 0x7ba7, 0x4ce6, 0xaa35, 0xf734c828a00e)

/* db1147e1-d5c9-41a0-be76-cd36e7d85572 */
#define SMART_SWITCH_UUID_GESTURE_CONFIG_CHAR_VAL \
	BT_UUID_128_ENCODE(0xdb1147e1, 0xd5c9, 0x41a0, 0xbe76, 0xcd36e7d85572)

/* fbdce744-27cf-46c3-a21b-84953dcd14da */
#define SMART_SWITCH_UUID_GESTURE_CONFIG_SCHEMA_CHAR_VAL \
	BT_UUID_128_ENCODE(0xfbdce744, 0x27cf, 0x46c3, 0xa21b, 0x84953dcd14da)

#define BT_UUID_SMART_SWITCH_GESTURE_CONFIG_SERVICE BT_UUID_DECLARE_128(SMART_SWITCH_UUID_GESTURE_CONFIG_SERVICE_VAL)
#define BT_UUID_SMART_SWITCH_GESTURE_CONFIG_CHAR BT_UUID_DECLARE_128(SMART_SWITCH_UUID_GESTURE_CONFIG_CHAR_VAL)
#define BT_UUID_SMART_SWITCH_GESTURE_CONFIG_SCHEMA_CHAR \
	BT_UUID_DECLARE_128(SMART_SWITCH_UUID_GESTURE_CONFIG_SCHEMA_CHAR_VAL)

/* Custom GATT service exposing the gesture engine's EngineConfig as a single read/write
 * characteristic (12-byte wire format, see ConfigStore). Statically present in the GATT
 * database; reads/writes are only accepted while BleConfigMode::IsActive() (i.e. while a
 * config-mode advertising window, opened by a 10s hold, is open). */
class GestureConfigService {
public:
	static void Init(GestureEngine &engine, const BleConfigMode &configMode);
};

}
