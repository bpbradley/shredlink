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

#define TILT_SENSOR	DT_LABEL(DT_NODELABEL(tilt0))

void main(void)
{
	const struct device *tilt;
	int ret;
	
	tilt = device_get_binding(TILT_SENSOR);
	if (tilt == NULL) {
		printk("Error getting tilt sensor device: %s\n", TILT_SENSOR);
	}

	while (1) {
		struct sensor_value tilted = {0};

		if (tilt != NULL){
			if ((ret = sensor_sample_fetch(tilt)) != 0){
				printk("tilt sensor fetch error: %d\n", ret);
			}
			else{
				sensor_channel_get(tilt, SENSOR_CHAN_TILT,
					&tilted);
			}
		}
		printk("Tilted=%d\n", tilted.val1);
		k_msleep(10);
	}
}
