
# True repo root: protocol/matter/src/app -> protocol/matter/src -> protocol/matter -> protocol -> root
set(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)

target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/app_task.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/matter/matter_switch_protocol.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/config/matter_persistent_store.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ble/ble_config_mode.cpp
    ${REPO_ROOT}/shared/gesture/gesture_engine.cpp
    ${REPO_ROOT}/shared/config/config_store.cpp
    ${REPO_ROOT}/shared/ble/gesture_config_service.cpp
)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${REPO_ROOT}
)
