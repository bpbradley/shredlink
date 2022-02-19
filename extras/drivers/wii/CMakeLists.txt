# Copyright (c) 2022 Brian Bradley
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_WII_PERIPHERAL_DRIVER)
zephyr_library()
zephyr_include_directories(include)
zephyr_library_sources_ifdef(CONFIG_WII_PERIPHERAL_DRIVER src/wii_peripheral.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE wii_driver_handlers.c)
endif()
