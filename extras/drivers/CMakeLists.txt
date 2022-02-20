# Copyright (c) 2022 Brian Bradley
# SPDX-License-Identifier: Apache-2.0
add_subdirectory_ifdef(CONFIG_SENSOR sensor)
add_subdirectory_ifdef(CONFIG_WII_PERIPHERAL_DRIVER wii)
