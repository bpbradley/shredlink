/**
 * @file gpio_tilt.c
 * @author Brian Bradley (brian.bradley.p@gmail.com)
 * @date 2022-02-24
 * 
 * @copyright Copyright (C) 2022 Brian Bradley
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#define DT_DRV_COMPAT gpio_tilt

#include <errno.h>
#include <zephyr.h>
#include <logging/log.h>
#include <drivers/sensor/tilt.h>
#include "gpio_tilt.h"

LOG_MODULE_REGISTER(tilt_gpio, CONFIG_SENSOR_LOG_LEVEL);

int tilt_sensor_read(const struct device *dev)
{
	struct gpio_tilt_data *data = dev->data;
	const struct gpio_tilt_config *cfg = dev->config;
	const struct device * tilt = data->tilt_gpio;
	int rc = gpio_pin_get(tilt, cfg->tilt_pin);
	if (rc < 0){
		return rc;
	}
	data->tilted = rc;
	return 0;
}

static int gpio_tilt_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_TILT);

	return tilt_sensor_read(dev);
}

static int gpio_tilt_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct gpio_tilt_data *data = dev->data;
	if (chan == (enum sensor_channel) SENSOR_CHAN_TILT){
		val->val1 = data->tilted;
		return 0;
	}
	return -ENOTSUP;
}

static const struct sensor_driver_api gpio_tilt_api_funcs = {
	.sample_fetch = gpio_tilt_fetch,
	.channel_get = gpio_tilt_get,
#ifdef CONFIG_TILT_TRIGGERS
	.attr_set = gpio_tilt_attr_set,
	.trigger_set = gpio_tilt_trigger_set,
#endif /* CONFIG_TILT_TRIGGERS */
};

int gpio_tilt_init(const struct device *dev)
{
	struct gpio_tilt_data *data = dev->data;
	const struct gpio_tilt_config *cfg = dev->config;

	if (!device_is_ready(data->tilt_gpio)) {
		LOG_ERR("GPIO device for tilt is not ready.");
		return -EINVAL;
	}
	/* Configure GPIO but don't turn it on */
	gpio_pin_configure(data->tilt_gpio, cfg->tilt_pin, GPIO_INPUT | cfg->tilt_flags);
	return 0;
}

static struct gpio_tilt_data gpio_tilt_data = {
	.tilt_gpio = DEVICE_DT_GET(DT_INST_PHANDLE(0, tilt_gpios)),
};

static const struct gpio_tilt_config gpio_tilt_cfg = {
	.tilt_pin = DT_INST_GPIO_PIN(0, tilt_gpios),
	.tilt_flags = DT_INST_GPIO_FLAGS(0, tilt_gpios),
	.tilt_controller = DT_INST_GPIO_LABEL(0, tilt_gpios),
};

DEVICE_DT_INST_DEFINE(0, gpio_tilt_init, NULL,
		      &gpio_tilt_data, &gpio_tilt_cfg, POST_KERNEL,
		      CONFIG_SENSOR_INIT_PRIORITY, &gpio_tilt_api_funcs);
