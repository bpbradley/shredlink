/**
 * @file main.c
 * @author Brian Bradley (brian.bradley.p@gmail.com)
 * @date 2022-02-18
 * 
 * @copyright Copyright (C) 2022 Brian Bradley
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#include <zephyr.h>
#include <logging/log.h>
#include <app_version.h>

LOG_MODULE_REGISTER(shredlink, CONFIG_SHREDLINK_LOG_LEVEL);

void main(void)
{
	LOG_INF("Starting %s version: %s", APP_NAME_STR, APP_VERSION_STR);
}
