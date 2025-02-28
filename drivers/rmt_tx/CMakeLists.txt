# SPDX-License-Identifier: Apache-2.0
zephyr_syscall_header(${ZEPHYR_T_WATCH_S3_MODULE_DIR}/include/zephyr/drivers/rmt_tx.h)
zephyr_library()
zephyr_library_sources_ifdef(CONFIG_RMT_TX_ESP32 rmt_tx_esp32.c)
