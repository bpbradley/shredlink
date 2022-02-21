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

struct polling_work_item{
    struct k_work_poll work;
    struct k_poll_signal signal;
    struct k_poll_event event;
};

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
        /* Data retrieved. Submit it for processing */
        submit_frame_data(data);
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
