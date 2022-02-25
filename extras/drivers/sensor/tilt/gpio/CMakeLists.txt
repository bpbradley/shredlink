# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(src/gpio_tilt.c)
zephyr_library_sources_ifdef(CONFIG_TILT_TRIGGERS gpio_tilt_triggers.c)
