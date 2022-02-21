/**
 * @file wii_peripheral.c
 * @author Brian Bradley (brian.bradley.p@gmail.com)
 * @date 2022-02-18
 * 
 * @copyright Copyright (C) 2022 Brian Bradley
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#define DT_DRV_COMPAT nintendo_wii

#include <errno.h>
#include <zephyr.h>
#include <device.h>
#include <kernel.h>
#include <drivers/i2c.h>
#include <init.h>
#include <sys/__assert.h>
#include <logging/log.h>
#include <wii.h>

LOG_MODULE_REGISTER(wii, CONFIG_WII_LOG_LEVEL);

/**
 * @brief Device driver data for wii peripheral
 * 
 */
struct wii_periph_data {
	const struct device *i2c_master;
	struct wii_btn_data wii;
};

/**
 * @brief configuration data for wii peripheral
 * 
 */
struct wii_periph_config {
	const char *i2c_bus;
	uint16_t i2c_addr;
    uint8_t i2c_speed;
};

static inline int wii_bus_config(const struct device *dev)
{
	const struct wii_periph_config *dev_cfg = dev->config;
	struct wii_periph_data *data = dev->data;
	return i2c_configure(data->i2c_master, I2C_MODE_MASTER | I2C_SPEED_SET(dev_cfg->i2c_speed));
}

static int wii_periph_poll_data(const struct device * dev, struct wii_btn_data * wii){
	if (wii == NULL){
		return -ENODEV;
	}
	const struct wii_periph_data *data = dev->data;
	const struct wii_periph_config *cfg = dev->config;
	uint8_t reg = 0;

	wii_bus_config(dev);
	i2c_write(data->i2c_master, &reg, 1, cfg->i2c_addr);
	int rc = i2c_read(data->i2c_master, wii->raw, 6, cfg->i2c_addr);
	LOG_HEXDUMP_DBG(wii->raw, 6, "wii raw data");
	return rc;
}

static int wii_periph_init(const struct device *dev)
{
    struct wii_periph_data *data = dev->data;
	const struct wii_periph_config *cfg = dev->config;
	struct wii_btn_data * wii = &data->wii;
	int rc = 0;
	
	memset(wii, 0, sizeof(struct wii_btn_data));

	data->i2c_master = device_get_binding(cfg->i2c_bus);
	if (!data->i2c_master) {
		LOG_DBG("i2c master not found: %s", cfg->i2c_bus);
		return -EINVAL;
	}

	return rc;
}

static struct wii_periph_data wii_periph_data;
static const struct wii_periph_config wii_periph_cfg = {
	.i2c_bus = DT_INST_BUS_LABEL(0),
	.i2c_addr = DT_INST_REG_ADDR(0),
	.i2c_speed = I2C_SPEED_FAST
};

static const struct wii_periph_driver_api wii_api_funcs = {
	.fetch = wii_periph_poll_data,
};

DEVICE_DT_INST_DEFINE(0, wii_periph_init, NULL,
		    &wii_periph_data, &wii_periph_cfg, POST_KERNEL,
		    CONFIG_APPLICATION_INIT_PRIORITY, &wii_api_funcs);
