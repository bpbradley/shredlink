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

#include <zephyr/kernel.h>
#include "gpio_tilt.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(tilt_gpio, CONFIG_SENSOR_LOG_LEVEL);

/**
 * @brief An interrupt should be handled by the application layer.
 * 
 * @param dev : pointer to device handle for sensor
 */
static void handle_int(const struct device *dev)
{
	struct gpio_tilt_data *data = dev->data;
	#if defined(CONFIG_TILT_SENSOR_TRIGGER_OWN_THREAD)
		k_sem_give(&data->sem);
	#elif defined(CONFIG_TILT_SENSOR_TRIGGER_GLOBAL_THREAD)
		k_work_submit(&data->work);
	#endif
}

#if CONFIG_TILT_SENSOR_MINIMUM_HOLD_TIME_MS > 0
/**
 * @brief Attempts to filter spurious or unintentional triggers.
 * 
 * This is the expiry function for `timer`. When the configured
 * hold time has expired, this function will execute. It will
 * check to see if the sensor has changed from active to inactive
 * (or vice versa) since the last time an interrupt was triggered.
 * 
 * If it has, then this will signal an interrupt. 
 * If there are no changes, and interrupt will not be triggered
 * as it will be considered spurios, so this event will be 
 * invisible to the application layer
 * 
 * @param timer : Timer which on expiry should execute this function
 */
static void filter_handler(struct k_timer * timer)
{
	struct gpio_tilt_data * data = timer->user_data;
	static int prev_state = -1;
	int state = gpio_pin_get_dt(&data->sensor);
	if (state != prev_state){
		handle_int(data->dev);
		prev_state = state;
	};
}
#endif
/**
 * @brief This will prepare an interrupt to be handled by the application.
 * 
 * If a minimum hold time is used for the sensor, such that activity is only
 * monitored on sufficiently long holds, then this will start a timer to
 * determine the legitimacy of this interrupt.
 * 
 * If no hold time is used (i.e. immediate access to data, including errant data)
 * then it will immediately handle the interrupt.
 * 
 * @param data : pointer to sensor data container
 */
static void prepare_int(struct gpio_tilt_data * data){
	#if CONFIG_TILT_SENSOR_MINIMUM_HOLD_TIME_MS > 0
		k_timer_start(&data->hold_timer, K_MSEC(CONFIG_TILT_SENSOR_MINIMUM_HOLD_TIME_MS), K_NO_WAIT);
	#else
		handle_int(data->dev);
	#endif
}

/**
 * @brief Either enables or disables interrupts on the monitored pin
 * 
 * @param dev : pointer to sensor device
 * @param enable 
 */
static inline void setup_int(const struct device *dev,
			     bool enable)
{
	const struct gpio_tilt_data *data = dev->data;
	unsigned int flags = enable
		? GPIO_INT_EDGE_BOTH
		: GPIO_INT_DISABLE;
	
	gpio_pin_interrupt_configure_dt(&data->sensor, flags);
}

/**
 * @brief This is where work is actually done for the interrupt,
 * including the application layer registered function.
 * 
 * @param dev : pointer to sensor device
 */
static void process_int(const struct device *dev)
{
	struct gpio_tilt_data *data = dev->data;

	if (data->trigger_handler) {
		data->trigger_handler(dev, &data->trig);
		setup_int(dev, true);
	}
}

/**
 * @brief register triggers from the application layers.
 * Here is where callbacks and types of triggers are selected.
 * 
 * @param dev : pointer to sensor device
 * @param trig : type of trigger to work on 
 * @param handler : handler function when a trigger occurs
 * @retval 0 on success
 * @retval -errno otherwise
 */
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
			prepare_int(data);
			rv = 0;
		}
	}
	return rv;
}

/**
 * @brief registered callback which is activated when a configured interrupt
 * on the sensor has occured
 * 
 * @param dev : pointer to gpio device
 * @param cb : callback
 * @param pins : unused
 */
static void alert_cb(const struct device *dev, struct gpio_callback *cb,
		     uint32_t pins)
{
	struct gpio_tilt_data *data =
		CONTAINER_OF(cb, struct gpio_tilt_data, alert_cb);

	ARG_UNUSED(pins);
	prepare_int(data);
}

#ifdef CONFIG_TILT_SENSOR_TRIGGER_OWN_THREAD

/**
 * @brief Dedicated work thread for processing interrupt data
 * 
 * @param data : pointer to sensor data
 */
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

/**
 * @brief Global system work thread for processing interrupt data
 * 
 * @param work : work item
 */
static void gpio_tilt_thread_cb(struct k_work *work)
{
	struct gpio_tilt_data *data =
		CONTAINER_OF(work, struct gpio_tilt_data, work);

	process_int(data->dev);
}

#endif /* CONFIG_TILT_SENSOR_TRIGGER_GLOBAL_THREAD */

/**
 * @brief Initialize trigger functionality
 * 
 * @param dev : pointer to sensor device
 * @retval 0 on success
 * @retval -errno otherwise
 */
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
	
	#if CONFIG_TILT_SENSOR_MINIMUM_HOLD_TIME_MS > 0
	/* Setup hold filter */
	k_timer_init(&data->hold_timer, filter_handler, NULL);
	data->hold_timer.user_data = (void *)data;
	#endif
	return rc;
}
