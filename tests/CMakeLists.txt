# Copyright (c) 2025, Noah Luskey <noah@vvvvvvvvvv.io>
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED)

project(bringup)
target_sources(app PRIVATE 
    src/main.c
    src/hello_world.c 
    src/power.c 
    src/haptics.c
    src/display.c
    src/touch.c
    src/imu.c
    src/rtc.c
    src/button.c
    src/charger.c
    src/fuel_gauge.c
    src/lora.c
    src/lorawan.c
    src/flash.c
)
