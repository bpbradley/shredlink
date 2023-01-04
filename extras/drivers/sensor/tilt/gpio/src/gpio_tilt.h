#ifndef SHREDLINK_DRIVERS_SENSOR_GPIO_TILT_H_
#define SHREDLINK_DRIVERS_SENSOR_GPIO_TILT_H_

#include <errno.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>

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
    #if CONFIG_TILT_SENSOR_MINIMUM_HOLD_TIME_MS > 0
        struct k_timer hold_timer;
    #endif
    #endif
};

#ifdef CONFIG_TILT_SENSOR_TRIGGER
int gpio_tilt_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);
int gpio_tilt_setup_interrupt(const struct device *dev);
#endif /* CONFIG_TILT_SENSOR_TRIGGER */

#endif
