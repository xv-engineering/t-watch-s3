# Copyright 2024 vvvvvvvvvv LLC, Noah Luskey <noah@vvvvvvvvvv.io>
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_include_directories(include)

zephyr_library_sources_ifdef(CONFIG_AXP2101 axp2101.c)
zephyr_library_sources_ifdef(CONFIG_REGULATOR_AXP2101 regulator_axp2101.c)
zephyr_library_sources_ifdef(CONFIG_GPIO_AXP2101 gpio_axp2101.c)
zephyr_library_sources_ifdef(CONFIG_CHARGER_AXP2101 charger_axp2101.c)


# Enabling CONFIG_REGULATOR often results in the drivers__charger library
# being built with no sources, which prints a warning message. Silence it
# by including a dummy file into that library.
if(CONFIG_REGULATOR)
target_sources(drivers__charger PRIVATE dummy.c)
endif()