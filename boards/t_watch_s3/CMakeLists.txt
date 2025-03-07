zephyr_library()

zephyr_library_sources(
    t_watch_s3_boot.c
)

# for access to the bma4xx sensor data format
# to properly hot-patch the sensor decode api
zephyr_library_include_directories(
    ${ZEPHYR_BASE}/drivers/sensor/bosch/bma4xx
)


