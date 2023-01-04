/**
 * @file wii_peripheral.h
 * @author Brian Bradley (brian.bradley.p@gmail.com)
 * @date 2022-02-18
 * 
 * @copyright Copyright (C) 2022 Brian Bradley
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#ifndef SHREDLINK_DRIVERS_SENSOR_NINTENDO_WII_H_
#define SHREDLINK_DRIVERS_SENSOR_NINTENDO_WII_H_

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief data structure containing all data retreived from the wii peripheral on fetch
 * 
 */

struct wii_btn_data {
    uint8_t raw[6];
};

typedef int (*wii_periph_api_fetch)(const struct device *dev, struct wii_btn_data * data);

__subsystem struct wii_periph_driver_api {
    wii_periph_api_fetch fetch;
};

__syscall int wii_peripheral_fetch(const struct device *dev, struct wii_btn_data * data);

static inline int z_impl_wii_peripheral_fetch(const struct device *dev, struct wii_btn_data * data)
{
	const struct wii_periph_driver_api *api =
				(struct wii_periph_driver_api *)dev->api;

	return api->fetch(dev, data);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/wii.h>

#endif