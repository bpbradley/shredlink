/**
 * @file hid.h
 * @author Brian Bradley (brian.bradley.p@gmail.com)
 * @date 2022-02-21
 * 
 * @copyright Copyright (C) 2022 Brian Bradley
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#ifndef __SHREDLINK_HID_H
#define __SHREDLINK_HID_H
#include <wii.h>

/**
 * @brief Submit a new frame to the queue to be processed
 * 
 * @param frame : frame data to be processed
 * @retval 0 on success
 * @retval -errno otherwise
 */
int submit_frame_data(struct wii_btn_data frame);

#endif