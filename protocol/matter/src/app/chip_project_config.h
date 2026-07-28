/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#define CHIP_CONFIG_CONTROLLER_MAX_ACTIVE_DEVICES 2

/* Default (platform/Zephyr/CHIPDevicePlatformConfig.h) is 0. Bumped to 2 (below BleConfigMode) 
 * so the config mode always wins over Matter's own automatic advertising */
#define CHIP_DEVICE_BLE_ADVERTISING_PRIORITY 2
