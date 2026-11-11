
target_sources_ifndef(CONFIG_DFU_TARGET_MCUBOOT app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/fake_bootloader.c
)
