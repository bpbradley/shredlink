/**
 * @file hid.c
 * @author Brian Bradley (brian.bradley.p@gmail.com)
 * @date 2022-02-21
 * 
 * @copyright Copyright (C) 2022 Brian Bradley
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <shredlink/hid.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_hid.h>

LOG_MODULE_DECLARE(shredlink, CONFIG_SHREDLINK_LOG_LEVEL);

#ifdef CONFIG_TILT_SENSOR
#define BTN_COUNT	10
#else
#define BTN_COUNT 9
#endif
static const uint8_t hid_report_desc[] = 
{
	HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),
	HID_USAGE(HID_USAGE_GEN_DESKTOP_GAMEPAD),
	HID_COLLECTION(HID_COLLECTION_APPLICATION),
	HID_COLLECTION(HID_COLLECTION_PHYSICAL),
			/* Bits used for button signalling */
			HID_USAGE_PAGE(HID_USAGE_GEN_BUTTON),
			HID_USAGE_MIN8(1),
			HID_USAGE_MAX8(BTN_COUNT),
			HID_LOGICAL_MIN8(0),
			HID_LOGICAL_MAX8(1),
			HID_REPORT_COUNT(BTN_COUNT),
			HID_REPORT_SIZE(1),
			/* HID_INPUT (Data,Var,Abs) */
			HID_INPUT(0x02),
			/* Unused bits */
			HID_REPORT_SIZE(16 - BTN_COUNT),
			HID_REPORT_COUNT(1),
			/* HID_INPUT (Cnst,Ary,Abs) */
			HID_INPUT(1),
			/* X and Y axis joystick */
			HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),
			HID_USAGE(HID_USAGE_GEN_DESKTOP_X),
			HID_USAGE(HID_USAGE_GEN_DESKTOP_Y),
			HID_LOGICAL_MIN8(0),
			HID_LOGICAL_MAX8(63),
			HID_REPORT_SIZE(8),
			HID_REPORT_COUNT(2),
			/* HID_INPUT (Data,Var,Abs) */
			HID_INPUT(0x02),
			/* whammy bar */
			HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),
            /* Slider object. May be a more appropriate usage */
			HID_USAGE(0x36),
			HID_LOGICAL_MIN8(0),
			HID_LOGICAL_MAX8(31),
			HID_REPORT_SIZE(8),
			HID_REPORT_COUNT(1),
			/* HID_INPUT (Data,Var,Abs) */
			HID_INPUT(0x02),
		HID_END_COLLECTION,
	HID_END_COLLECTION,
};

/**
 * @brief Pack the hid report so that it can be sent raw
 * rather than packing into a coniguous array manually.
 * 
 */
struct __attribute__((packed)) hid_report{
	uint16_t buttons;
	uint8_t axes[2];
	uint8_t whammy;
};

K_MSGQ_DEFINE(hid_msgq, sizeof(struct hid_report), 5, 4);

/**
 * @brief Packs gamepad data into the prepared hid report format
 * 
 * @param rpt : pointer to hid report to fill
 * @param data : pointer to data frame with which to fill the report
 * @retval -ENODEV on rpt or data NULL
 * @retval 0 on success
 */
static int fill_hid_report(struct hid_report * rpt, struct gamepad * data){
	if (rpt == NULL || data == NULL){
		return -ENODEV;
	}
	rpt->axes[0] = data->axes[0];
	rpt->axes[1] = data->axes[1];
	rpt->whammy = data->axes[2];
	rpt->buttons = data->buttons;
	return 0;
}

int submit_frame_data(struct gamepad * frame){
    struct hid_report report;
    int ret = fill_hid_report(&report, frame);
    if (ret == 0){
            while ((ret = k_msgq_put(&hid_msgq, &report, K_NO_WAIT)) != 0) {
            /* message queue is full: purge old data & try again */
            k_msgq_purge(&hid_msgq);
        }
    }
    return ret;
}

static enum usb_dc_status_code usb_status;
static void status_cb(enum usb_dc_status_code status, const uint8_t *param)
{
	usb_status = status;
}

void hid_process(void){
    const struct device *hid;
	int ret;
    hid = device_get_binding("HID_0");
	if (hid == NULL) {
		LOG_ERR("Cannot get USB HID Device");
		return;
	}
	usb_hid_register_device(hid,
				hid_report_desc, sizeof(hid_report_desc),
				NULL);

	usb_hid_init(hid);

	ret = usb_enable(status_cb);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return;
	}
	struct hid_report last_report = {0};
    while(1){
        /* Grab the latest report when it is available,
        and send it if the data is new */
        struct hid_report report = {0};
        if (k_msgq_get(&hid_msgq, &report, K_FOREVER) == 0)
		{
            /* Check if the report is new */
            if (memcmp(&report, &last_report, sizeof(struct hid_report)) != 0){
				/* Data doesn't match. Immediately send an update to the host. */
				ret = hid_int_ep_write(hid, (const uint8_t *)&report, sizeof(report), NULL);
				if (ret) {
					LOG_ERR("HID write error, %d", ret);
				}
                /* Update the latest data */
			    memcpy(&last_report, &report, sizeof(struct hid_report));
			}
        }
    }
}

K_THREAD_DEFINE(hid_reporting, CONFIG_SHREDLINK_HID_STACKSIZE, hid_process, 
	NULL, NULL, NULL, CONFIG_SHREDLINK_HID_PRIORITY, 0, 0);
