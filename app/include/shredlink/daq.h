/**
 * @file daq.h
 * @author Brian Bradley (brian.bradley.p@gmail.com)
 * @date 2022-02-20
 * 
 * @copyright Copyright (C) 2022 Brian Bradley
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#ifndef __SHREDLINK_POLLER_H
#define __SHREDLINK_POLLER_H

#include <zephyr/kernel.h>

/**
 * @TODO: this should be configurable and part of the gamepad API
 * 
 */
struct gamepad{
    uint32_t buttons;
    uint8_t axes[3];
};

#ifdef CONFIG_GAMEPAD_DAQ_POLL_MODE
/**
 * @brief Thread which controls the data acquisition process in polling mode
 * 
 */
void gamepad_polling_process(void);

#endif
/**
 * @brief Indicate that a change in tilt has been detected.
 * 
 * This will overwrite any previous event which was signaled
 * but had not yet been consumed.
 * 
 * @param tilt : boolean indication on if a tilt event has occured
 * @retval 0 on success
 * @retval -errno otherwise
 */
int signal_tilt_event(bool tilt);

#endif
