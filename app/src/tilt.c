/**
 * @file tilt.c
 * @author Brian Bradley (brian.bradley.p@gmail.com)
 * @date 2022-02-26
 * 
 * @copyright Copyright (C) 2022 Brian Bradley
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 */
#include <zephyr.h>
#include <logging/log.h>
#include <device.h>
#include <drivers/sensor.h>
#include <drivers/sensor/tilt.h>
#include <shredlink/daq.h>

LOG_MODULE_DECLARE(shredlink, CONFIG_SHREDLINK_LOG_LEVEL);

#define TILT_SENSOR	DT_NODELABEL(tilt0)

/**
 * @brief Handler function when an interrupt is triggered on the tilt sensor
 * 
 * @param dev 
 * @param trig 
 */
static void tilt_trigger_handler(const struct device *dev,
			    const struct sensor_trigger *trig)
{  
	struct sensor_value tilt;
	int rc;
	rc = sensor_sample_fetch(dev);
	if (rc != 0) {
		LOG_DBG("tilt sensor fetch error: %d", rc);
		return;
	}
	rc = sensor_channel_get(dev, SENSOR_CHAN_TILT, &tilt);
	if (rc != 0) {
		LOG_DBG("tilt sensor get error: %d", rc);
		return;
	}
    signal_tilt_event(tilt.val1);
}

/**
 * @brief Setup the tilt sensor
 * 
 * @retval 0 on success
 * @retval -errno otherwise
 */
static int configure_tilt_sensor(){
    const struct device *tilt = DEVICE_DT_GET(TILT_SENSOR);
	int rc = 0;
	static struct sensor_trigger trig;
	trig.type = SENSOR_TRIG_THRESHOLD;
	trig.chan = SENSOR_CHAN_TILT;
	rc = sensor_trigger_set(tilt, &trig, tilt_trigger_handler);
	if (rc != 0) {
		LOG_ERR("Trigger set failed: %d", rc);
	}
	/* Manually trigger the handler to populate initial values */
	tilt_trigger_handler(tilt, &trig);
    return rc;
}

SYS_INIT(configure_tilt_sensor, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);