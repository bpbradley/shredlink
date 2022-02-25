#ifndef SHREDLINK_DRIVERS_SENSOR_GPIO_TILT_H_
#define SHREDLINK_DRIVERS_SENSOR_GPIO_TILT_H_

#include <errno.h>
#include <zephyr/types.h>
#include <device.h>
#include <drivers/sensor.h>
#include <sys/util.h>
#include <drivers/gpio.h>

struct gpio_tilt_data {
	const struct device *tilt_gpio;
    int tilted;
    
    #ifdef CONFIG_TILT_TRIGGERS
        struct gpio_callback alert_cb;
        const struct device *dev;
        struct sensor_trigger trig;
        sensor_trigger_handler_t trigger_handler;
    
    #ifdef CONFIG_TILT_TRIGGERS_OWN_THREAD
        struct k_sem sem;
    #endif
    #ifdef CONFIG_TILT_TRIGGERS_GLOBAL_THREAD
        struct k_work work;
    #endif

    #endif
};

struct gpio_tilt_config {
	uint8_t tilt_pin;
	uint8_t tilt_flags;
	const char *tilt_controller;
};
#endif
