# Copyright 2024 vvvvvvvvvv LLC, Noah Luskey <noah@vvvvvvvvvv.io>
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_include_directories(include)

zephyr_library_sources_ifdef(CONFIG_AXP2101 axp2101.c)
zephyr_library_sources_ifdef(CONFIG_REGULATOR_AXP2101 regulator_axp2101.c)
zephyr_library_sources_ifdef(CONFIG_GPIO_AXP2101 gpio_axp2101.c)