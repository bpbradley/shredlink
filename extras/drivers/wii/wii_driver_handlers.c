/**
 * @file wii_driver_handlers.c
 * @author Brian Bradley (brian.bradley.p@gmail.com)
 * @date 2022-02-19
 * 
 * @copyright Copyright (C) 2022 Brian Bradley
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#include <syscall_handler.h>
#include <wii.h>
#include <kernel.h>
#include <syscall_list.h>

static inline int z_vrfy_wii_peripheral_fetch(const struct device *dev, struct wii_btn_data * data)
{ 
    Z_OOPS(Z_SYSCALL_DRIVER_WII_PERIPHERAL_DRIVER(dev, fetch)); 
    return z_impl_wii_peripheral_fetch((const struct device *)dev, data); 
}
#include <syscalls/wii_peripheral_fetch_mrsh.c>
