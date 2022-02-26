/**
 * @file main.c
 * @author Brian Bradley (brian.bradley.p@gmail.com)
 * @date 2022-02-25
 * 
 * @copyright Copyright (C) 2022 Brian Bradley
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#include <zephyr.h>
#include <ztest.h>
#include <drivers/sensor.h>
#include <drivers/sensor/tilt.h>

#define TILT_SENSOR_LABEL DT_LABEL(DT_NODELABEL(tilt0))
const struct device *get_tilt_sensor_device(void){
    const struct device * dev = device_get_binding(TILT_SENSOR_LABEL);
    zassert_not_null(dev, "failed: dev '%s' is null", TILT_SENSOR_LABEL);
    return dev;
}

static void test_get_sensor_value(int16_t channel){
    struct sensor_value value;
	const struct device *dev = get_tilt_sensor_device();
	zassert_ok(sensor_sample_fetch_chan(dev, channel), "Sample fetch failed");
	zassert_ok(sensor_channel_get(dev, channel, &value), "Get sensor value failed");
	zassert_true(value.val1 == 0 || value.val1 == 1, "Tilt sensor value should be 0 or 1");
}

void test_get_sensor_value_not_supp(int16_t channel)
{
	const struct device *dev = get_tilt_sensor_device();

	zassert_true(sensor_sample_fetch_chan(dev, channel) == -ENOTSUP, "Unsupported channels should return -ENOTSUP");
}

static void test_unspported_channel(void){

	/* for all channels */
	for (int c = 0; c < SENSOR_CHAN_ALL; c++){
		/* All standard channels are unsupported */
		test_get_sensor_value_not_supp(c);
	}
}

static void test_get_tilt(void){
    test_get_sensor_value(SENSOR_CHAN_TILT);
}

#ifdef CONFIG_TILT_SENSOR_TRIGGER
static void tilt_trigger_handler(const struct device *dev,
			    struct sensor_trigger *trig)
{
	return;
}
#endif

static void test_trigger(void){
	#ifdef CONFIG_TILT_SENSOR_TRIGGER
		static struct sensor_trigger trig;
		const struct device *dev = get_tilt_sensor_device();
		trig.type = SENSOR_TRIG_THRESHOLD;
		trig.chan = SENSOR_CHAN_TILT;
		zassert_ok(sensor_trigger_set(dev, &trig, tilt_trigger_handler), "Unable to configure trigger");
	#endif
}

void test_main(void)
{
    ztest_test_suite(tilt_sensor_tests,
		ztest_unit_test(test_unspported_channel),
        ztest_unit_test(test_get_tilt),
		ztest_unit_test(test_trigger)
	);
	ztest_run_test_suite(tilt_sensor_tests);
}
