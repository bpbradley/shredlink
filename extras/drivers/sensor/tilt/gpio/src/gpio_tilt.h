#ifndef SHREDLINK_DRIVERS_SENSOR_GPIO_TILT_H_
#define SHREDLINK_DRIVERS_SENSOR_GPIO_TILT_H_

#include <errno.h>
#include <zephyr/types.h>
#include <device.h>
#include <drivers/sensor.h>
#include <sys/util.h>
#include <drivers/gpio.h>

struct gpio_tilt_data {
	struct gpio_dt_spec sensor;
    int status;
    
    #ifdef CONFIG_TILT_SENSOR_TRIGGER
        struct gpio_callback alert_cb;
        const struct device *dev;
        struct sensor_trigger trig;
        sensor_trigger_handler_t trigger_handler;
    
    #ifdef CONFIG_TILT_SENSOR_TRIGGER_OWN_THREAD
        struct k_sem sem;
    #endif
    #ifdef CONFIG_TILT_SENSOR_TRIGGER_GLOBAL_THREAD
        struct k_work work;
    #endif

    #endif
};

struct gpio_tilt_config {
	const char *tilt_controller;
};

#ifdef CONFIG_TILT_SENSOR_TRIGGER
int gpio_tilt_attr_set(const struct device *dev, enum sensor_channel chan,
		     enum sensor_attribute attr,
		     const struct sensor_value *val);
int gpio_tilt_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);
int gpio_tilt_setup_interrupt(const struct device *dev);
#endif /* CONFIG_TILT_SENSOR_TRIGGER */

#endif
