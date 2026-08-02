#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(smart-switch-zigbee)

set(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

target_sources(app PRIVATE
    src/main.cpp
    src/zigbee/zigbee_switch_protocol.cpp
    src/zigbee/zigbee_ble_config_mode.cpp
    src/zigbee/zigbee_persistent_store.cpp
    src/zigbee/zigbee_factory_reset.cpp
    src/zigbee/zigbee_network_removal.cpp
    src/zigbee/zigbee_network_info.cpp
    src/zigbee/zigbee_gesture_config_attrs.cpp
    ${REPO_ROOT}/shared/gesture/gesture_engine.cpp
    ${REPO_ROOT}/shared/config/config_store.cpp
    ${REPO_ROOT}/shared/ble/gesture_config_service.cpp
    ${REPO_ROOT}/shared/diag/device_log.cpp
    ${REPO_ROOT}/shared/diag/device_log_backend.c
)

target_include_directories(app PRIVATE
    src
    ${REPO_ROOT}
)
