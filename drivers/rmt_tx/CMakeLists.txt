# SPDX-License-Identifier: Apache-2.0
zephyr_syscall_header(${ZEPHYR_T_WATCH_S3_MODULE_DIR}/include/zephyr/drivers/rmt_tx.h)
zephyr_library()
add_subdirectory_ifdef(CONFIG_RMT_TX_ESP32 esp32)
