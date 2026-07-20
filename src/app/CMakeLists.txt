
target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/app_task.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gesture/gesture_engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/matter/matter_switch_protocol.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/config/config_store.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ble/ble_config_mode.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ble/gesture_config_service.cpp
)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
