/**
 * @file main.c
 * @date 2022-02-24
 * 
 * @copyright Copyright (C) 2022 Brian Bradley
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#include <zephyr.h>
#include <drivers/sensor.h>
#include <drivers/sensor/tilt.h>
#include <drivers/gpio.h>

#define LED_ID	DT_ALIAS(led0)
#define TILT_SENSOR	DT_NODELABEL(tilt0)
#define LED_SPEC(node_id) GPIO_DT_SPEC_GET_OR(node_id, gpios, {0})

#ifdef CONFIG_TILT_SENSOR_TRIGGER
static void tilt_trigger_handler(const struct device *dev,
			    struct sensor_trigger *trig)
{
	struct sensor_value tilt;
	struct gpio_dt_spec led = LED_SPEC(LED_ID);
	int rc;
	rc = sensor_sample_fetch(dev);
	if (rc != 0) {
		printk("tilt sensor fetch error: %d\n", rc);
		return;
	}
	rc = sensor_channel_get(dev, SENSOR_CHAN_TILT, &tilt);
	if (rc != 0) {
		printk("tilt sensor get error: %d\n", rc);
		return;
	}
	printk("Triggered Sample=%d\n", tilt.val1);
	if (led.port){
		gpio_pin_set_dt(&led, tilt.val1);
	}
}
#endif

void main(void)
{
	const struct device *tilt = DEVICE_DT_GET(TILT_SENSOR);
	struct gpio_dt_spec led = LED_SPEC(LED_ID);
	int rc;

	if (led.port){
		gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	}

	#ifdef CONFIG_TILT_SENSOR_TRIGGER
	static struct sensor_trigger trig;
	trig.type = SENSOR_TRIG_THRESHOLD;
	trig.chan = SENSOR_CHAN_TILT;
	rc = sensor_trigger_set(tilt, &trig, tilt_trigger_handler);
	if (rc != 0) {
		printk("Trigger set failed: %d\n", rc);
		return;
	}
	#endif

	while (1) {
		struct sensor_value tilted = {0};

		if (tilt != NULL){
			if ((rc = sensor_sample_fetch(tilt)) != 0){
				printk("tilt sensor fetch error: %d\n", rc);
			}
			else{
				sensor_channel_get(tilt, SENSOR_CHAN_TILT,
					&tilted);
			}
		}
		printk("Polled Sample=%d\n", tilted.val1);
		if (led.port){
			gpio_pin_set_dt(&led, tilted.val1);
		}
		k_msleep(20000);
	}
}
