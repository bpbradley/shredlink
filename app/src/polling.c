/**
 * @file polling.c
 * @author Brian Bradley (brian.bradley.p@gmail.com)
 * @brief Poll gamepad for new data when in polling acquisiton mode.
 * @date 2022-02-20
 * 
 * @copyright Copyright (C) 2022 Brian Bradley
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 */
#include <zephyr.h>
#include <wii.h>
#include <device.h>
#include <sys/util.h>
#include <logging/log.h>
#include <shredlink/daq.h>
#include <shredlink/hid.h>

LOG_MODULE_DECLARE(shredlink, CONFIG_SHREDLINK_LOG_LEVEL);

K_EVENT_DEFINE(tilt_ev);

#define EVENT_TILT_ACTIVE	BIT(0)
#define EVENT_TILT_INACTIVE	BIT(1)

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

struct polling_work_item{
    struct k_work_poll work;
    struct k_poll_signal signal;
    struct k_poll_event event;
};

int signal_tilt_event(bool tilt){
	uint32_t event = tilt ? EVENT_TILT_ACTIVE : EVENT_TILT_INACTIVE;
	k_event_set(&tilt_ev, event);
	return 0;
}

static int pack_gamepad_data(struct gamepad * packed, struct wii_btn_data frame, int32_t tilt){
	if (packed == NULL){
		return -ENODEV;
	}
    wii_fmt_t fmt = {
        .frame = frame
    };
	packed->axes[0] = fmt.guitar.analog_x;
	packed->axes[1] = fmt.guitar.analog_y;
	packed->axes[2] = fmt.guitar.whammy;
	packed->buttons = fmt.guitar.neck;
	WRITE_BIT(packed->buttons, 5, fmt.guitar.button_plus);
	WRITE_BIT(packed->buttons, 6, fmt.guitar.button_minus);
	WRITE_BIT(packed->buttons, 7, fmt.guitar.strum_up);
	WRITE_BIT(packed->buttons, 8, fmt.guitar.strum_down);
    WRITE_BIT(packed->buttons, 9, !tilt);

    /**
     * @TODO: The logic level is inverted on the wii guitar I am testing with.
     * This should be handled in the gamepad API instead of the application layer.
     * 
     */
	packed->buttons ^= UINT32_MAX; /* logic level is inverted */
	LOG_DBG("report: x: %d y: %d whammy: %d buttons: %02x", packed->axes[0], packed->axes[1], packed->axes[2], packed->buttons);
	return 0;
}

/**
 * @brief work process which handles data acquisition
 * and submission of a single data frame.
 * 
 * @param work : work queue entry item
 */
static void poll_work_item(struct k_work *work){
    const struct device *wii = DEVICE_DT_GET(DT_NODELABEL(wii_guitar));
    int ret = 0;
    struct wii_btn_data data = {0};
    if ((ret = wii_peripheral_fetch(wii, &data)) != 0){
        LOG_ERR("gamepad fetch error: %d", ret);
    }
    else{
        /* Data retrieved. Check if tilt data became available */
		uint32_t events;
		static int32_t tilt = 0;
		events = k_event_wait(&tilt_ev, 
			EVENT_TILT_ACTIVE | EVENT_TILT_INACTIVE, 
			false, K_NO_WAIT);
		if (events & EVENT_TILT_ACTIVE){
			tilt = 1;
		}
		else if (events & EVENT_TILT_INACTIVE){
			tilt = 0;
		}
		/* Pack the data and submit it for output */
        struct gamepad gamepad;
        pack_gamepad_data(&gamepad, data, tilt);
        submit_frame_data(&gamepad);
    }
}

void gamepad_polling_process(void){
    static struct polling_work_item work_item;

    k_work_poll_init(&work_item.work, poll_work_item);
    k_poll_signal_init(&work_item.signal);
    k_poll_event_init(&work_item.event, 
                    K_POLL_TYPE_SIGNAL,
                    K_POLL_MODE_NOTIFY_ONLY,
                    &work_item.signal);

	while (1) {
        /* Submit work to the system workqueue to be processed in parallel
        to the waiting process. This way, any process latency associated
        with data acquisition and submission is fully decoupled from the
        requested poll rate. */
        k_work_poll_submit(&work_item.work, &work_item.event, 1, K_NO_WAIT);
		k_usleep(USEC_PER_SEC / CONFIG_GAMEPAD_POLL_RATE_HZ);
	}
}

K_THREAD_DEFINE(poller, CONFIG_SHREDLINK_DAQ_STACKSIZE, gamepad_polling_process, 
	NULL, NULL, NULL, CONFIG_SHREDLINK_DAQ_PRIORITY, 0, CONFIG_SHREDLINK_DAQ_START_DELAY);
