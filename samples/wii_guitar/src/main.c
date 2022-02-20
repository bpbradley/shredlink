/**
 * @file main.c
 * @author Brian Bradley (brian.bradley.p@gmail.com)
 * @date 2022-02-18
 * 
 * @copyright Copyright (C) 2022 Brian Bradley
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#include <zephyr.h>
#include <wii.h>
#include <string.h>

#define WII_GUITAR	DT_LABEL(DT_NODELABEL(wii_guitar))

struct __attribute__((packed)) guitar_data{
	uint8_t analog_x: 6;
	uint8_t gh0: 2;
	uint8_t analog_y: 6;
	uint8_t gh1: 2;
	uint8_t touchbar: 5;
	uint8_t empty0: 3;
	uint8_t whammy: 5;
	uint8_t empty1: 3;
	uint16_t buttons: 16;
};

typedef union guitar_buttons{
	struct wii_btn_data frame;
	struct guitar_data guitar;
}buttons_t;

void main(void)
{
	const struct device *wii;
	int ret;

	wii = device_get_binding(WII_GUITAR);
	if (wii == NULL) {
		printk("Error getting gas sensor device: %s\n", WII_GUITAR);
	}

	while (1) {
		buttons_t data = {0};
		if (wii != NULL){
			if ((ret = wii_peripheral_fetch(wii, &data.frame)) != 0){
				printk("wii peripheral fetch error: %d\n", ret);
			}
			else{
				printk("\nraw data: ");
				for (int i = 0; i < sizeof(data.frame.raw) / sizeof(uint8_t); ++i){
					printk("%02x ", data.frame.raw[i]);
				}
				printk("\n");
				printk("x: %d y: %d whammy: %d buttons: %04x\n", \
					data.guitar.analog_x, data.guitar.analog_y, data.guitar.whammy, data.guitar.buttons);
			}
		}
		k_usleep(500);
	}
}
