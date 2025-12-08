
target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/app_task.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/light_switch.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/shell_commands.cpp
    ${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/src/binding/binding_handler.cpp
)