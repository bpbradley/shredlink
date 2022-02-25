/**
 * @file gpio_tilt_triggers.c
 * @author Brian Bradley (brian.bradley.p@gmail.com)
 * @date 2022-02-24
 * 
 * @copyright Copyright (C) 2022 Brian Bradley
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#include <zephyr.h>
#include "gpio_tilt.h"
#include <logging/log.h>

LOG_MODULE_DECLARE(tilt_gpio, CONFIG_SENSOR_LOG_LEVEL);


int gpio_tilt_attr_set(const struct device *dev, enum sensor_channel chan,
		     enum sensor_attribute attr,
		     const struct sensor_value *val)
{
	return 0;
}

static inline void setup_int(const struct device *dev,
			     bool enable)
{
	const struct gpio_tilt_data *data = dev->data;
	unsigned int flags = enable
		? GPIO_INT_EDGE_BOTH
		: GPIO_INT_DISABLE;
	
	gpio_pin_interrupt_configure_dt(&data->sensor, flags);
}

static void handle_int(const struct device *dev)
{
	struct gpio_tilt_data *data = dev->data;
	
#if defined(CONFIG_TILT_SENSOR_TRIGGER_OWN_THREAD)
	k_sem_give(&data->sem);
#elif defined(CONFIG_TILT_SENSOR_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static void process_int(const struct device *dev)
{
	struct gpio_tilt_data *data = dev->data;

	if (data->trigger_handler) {
		data->trigger_handler(dev, &data->trig);
	}

	if (data->trigger_handler) {
		setup_int(dev, true);
	}
}

int gpio_tilt_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct gpio_tilt_data *data = dev->data;
	int rv = 0;

	setup_int(dev, false);

	data->trig = *trig;
	data->trigger_handler = handler;

	if (handler != NULL) {
		setup_int(dev, true);

		rv = gpio_pin_get_dt(&data->sensor);
		if (rv >= 0) {
			handle_int(dev);
			rv = 0;
		}
	}
	return rv;
}

static void alert_cb(const struct device *dev, struct gpio_callback *cb,
		     uint32_t pins)
{
	struct gpio_tilt_data *data =
		CONTAINER_OF(cb, struct gpio_tilt_data, alert_cb);

	ARG_UNUSED(pins);
	handle_int(data->dev);
}

#ifdef CONFIG_TILT_SENSOR_TRIGGER_OWN_THREAD

static void gpio_tilt_thread_main(struct gpio_tilt_data *data)
{
	while (true) {
		k_sem_take(&data->sem, K_FOREVER);
		process_int(data->dev);
	}
}

static K_KERNEL_STACK_DEFINE(gpio_tilt_thread_stack, CONFIG_TILT_SENSOR_THREAD_STACK_SIZE);
static struct k_thread gpio_tilt_thread;
#else /* CONFIG_TILT_SENSOR_TRIGGER_GLOBAL_THREAD */

static void gpio_tilt_thread_cb(struct k_work *work)
{
	struct gpio_tilt_data *data =
		CONTAINER_OF(work, struct gpio_tilt_data, work);

	process_int(data->dev);
}

#endif /* CONFIG_TILT_SENSOR_TRIGGER_GLOBAL_THREAD */

int gpio_tilt_setup_interrupt(const struct device *dev)
{
	struct gpio_tilt_data *data = dev->data;
	const struct device *tilt = data->sensor.port;
	data->dev = dev;

#ifdef CONFIG_TILT_SENSOR_TRIGGER_OWN_THREAD
	k_sem_init(&data->sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&gpio_tilt_thread, gpio_tilt_thread_stack,
			CONFIG_TILT_SENSOR_THREAD_STACK_SIZE,
			(k_thread_entry_t)gpio_tilt_thread_main, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_TILT_SENSOR_THREAD_PRIORITY),
			0, K_NO_WAIT);
#else /* CONFIG_TILT_SENSOR_TRIGGER_GLOBAL_THREAD */
	data->work.handler = gpio_tilt_thread_cb;
#endif /* trigger type */

    gpio_init_callback(&data->alert_cb, alert_cb, BIT(data->sensor.pin));
    int rc = gpio_add_callback(tilt, &data->alert_cb);

	return rc;
}
