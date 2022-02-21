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

#include <zephyr.h>

#ifdef CONFIG_GAMEPAD_DAQ_POLL_MODE
/**
 * @brief Thread which controls the data acquisition process in polling mode
 * 
 */
void gamepad_polling_process(void);
#endif
#endif
