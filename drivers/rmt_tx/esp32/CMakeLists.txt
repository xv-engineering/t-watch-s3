zephyr_library_sources(
    rmt_tx_esp32.c 
    rmt_tx_esp32_channel.c 
)

# the esp32 module does not include the rmt hal sources.
# In the interest of keeping this module out-of-tree,
# we manually add the missing source here.
zephyr_library_sources(
    ${ZEPHYR_HAL_ESPRESSIF_MODULE_DIR}/components/hal/rmt_hal.c
)

