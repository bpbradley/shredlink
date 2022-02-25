/**
 * @file tilt.h
 * @author Brian Bradley (brian.bradley.p@gmail.com)
 * @brief Extended public API for tilt sensors
 * @date 2022-02-24
 * 
 * @copyright Copyright (C) 2022 Brian Bradley
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#ifndef SHREDLINK_INCLUDE_DRIVERS_SENSOR_TILT_H_
#define SHREDLINK_INCLUDE_DRIVERS_SENSOR_TILT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <drivers/sensor.h>

enum sensor_channel_tilt {
    /**
	 * Tilt.  Adimensional.  A value of 1 indicates that the
     * sensor is tilted
	 */
	SENSOR_CHAN_TILT = SENSOR_CHAN_PRIV_START,
};

#ifdef __cplusplus
}
#endif

#endif /* SHREDLINK_INCLUDE_DRIVERS_SENSOR_TILT_H_ */


