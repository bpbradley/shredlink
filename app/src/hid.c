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
#include <zephyr.h>
#include <device.h>
#include <logging/log.h>
#include <shredlink/hid.h>
#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>

LOG_MODULE_DECLARE(shredlink, CONFIG_SHREDLINK_LOG_LEVEL);

/**
 * @TODO: This will eventually be moved into the gamepad api
 * 
 */
struct __attribute__((packed)) guitar_data{
	uint8_t analog_x: 6;
	uint8_t gh0: 2;
	uint8_t analog_y: 6;
	uint8_t gh1: 2;
	uint8_t touchbar: 5;
	uint8_t empty0: 3;
	uint8_t whammy: 5;
	uint8_t empty1: 3;
	uint8_t empty2: 2;
	uint8_t button_plus: 1;
	uint8_t empty3: 1;
	uint8_t button_minus: 1;
	uint8_t empty4: 1;
	uint8_t strum_down: 1;
	uint8_t empty5: 1;
	uint8_t strum_up: 1;
	uint8_t empty6: 2;
	uint8_t neck: 5;
};

static const uint8_t hid_report_desc[] = 
{
	HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),
	HID_USAGE(HID_USAGE_GEN_DESKTOP_GAMEPAD),
	HID_COLLECTION(HID_COLLECTION_APPLICATION),
	HID_COLLECTION(HID_COLLECTION_PHYSICAL),
			/* Bits used for button signalling */
			HID_USAGE_PAGE(HID_USAGE_GEN_BUTTON),
			HID_USAGE_MIN8(1),
			HID_USAGE_MAX8(9),
			HID_LOGICAL_MIN8(0),
			HID_LOGICAL_MAX8(1),
			HID_REPORT_COUNT(9),
			HID_REPORT_SIZE(1),
			/* HID_INPUT (Data,Var,Abs) */
			HID_INPUT(0x02),
			/* Unused bits */
			HID_REPORT_SIZE(7),
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
 * @brief This union is a conveient container which allows
 * easily converting between raw frame data, and useful
 * gamepad data without needing to mask and shift everything
 * manually.
 * 
 */
typedef union wii_data_fmt{
	struct wii_btn_data frame;
	struct guitar_data guitar;
}wii_fmt_t;

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
static int fill_hid_report(struct hid_report * rpt, wii_fmt_t * data){
	if (rpt == NULL || data == NULL){
		return -ENODEV;
	}
	rpt->axes[0] = data->guitar.analog_x;
	rpt->axes[1] = data->guitar.analog_y;
	rpt->whammy = data->guitar.whammy;
	rpt->buttons = data->guitar.neck;
	WRITE_BIT(rpt->buttons, 5, data->guitar.button_plus);
	WRITE_BIT(rpt->buttons, 6, data->guitar.button_minus);
	WRITE_BIT(rpt->buttons, 7, data->guitar.strum_up);
	WRITE_BIT(rpt->buttons, 8, data->guitar.strum_down);

    /**
     * @TODO: The logic level is inverted on the wii guitar I am testing with.
     * This should be handled in the gamepad API instead of the application layer.
     * 
     */
	rpt->buttons ^= 0xffff; /* logic level is inverted */
	LOG_DBG("report: x: %d y: %d whammy: %d buttons: %02x", rpt->axes[0], rpt->axes[1], rpt->whammy, rpt->buttons);
	return 0;
}

int submit_frame_data(struct wii_btn_data frame){
    struct hid_report report;
    wii_fmt_t fmt = {
        .frame = frame
    };
    int ret = fill_hid_report(&report, &fmt);
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
