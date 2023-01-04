/**
 * @file wii_peripheral.c
 * @author Brian Bradley (brian.bradley.p@gmail.com)
 * @date 2022-02-18
 * 
 * @copyright Copyright (C) 2022 Brian Bradley
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#define DT_DRV_COMPAT nintendo_wii

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <wii.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(wii, CONFIG_WII_LOG_LEVEL);

typedef enum WiiTypes{
	WII_NO_DEVICE,
	WII_CLASSIC,
	WII_NUNCHUK,
	WII_CLASSIC_PRO,
	WII_GUITAR,
	WII_DRUMS,
	WII_TURNTABLE,
}wii_type_t;

/**
 * @brief Data associated with a specific peripheral implementation.
 */
struct wii_peripheral{
	wii_type_t peripheral;
	uint64_t id; /* This is the decrypted id, so the data stream must first be unencrypted before matching */
	const char * label;
};

/**
 * @brief Device driver data for wii peripheral
 * 
 */
struct wii_periph_data {
	struct wii_btn_data wii;
	const struct wii_peripheral * peripheral;
};

/**
 * @brief configuration data for wii peripheral
 * 
 */
struct wii_periph_config {
	struct i2c_dt_spec i2c;
    uint8_t speed;
};

struct cmd_seq_item {
	const uint8_t reg;
	const uint8_t data;
};

#define DEFINE_WII_PERIPHERAL(_tag, _id, _label)	\
	{.peripheral = _tag, .id = (__bswap_48((uint64_t)_id)), .label=_label}

const struct wii_peripheral wii_peripheral_table[] = {
	DEFINE_WII_PERIPHERAL(WII_CLASSIC, 0xa4200101, "Wii Classic Controller"),
	DEFINE_WII_PERIPHERAL(WII_NUNCHUK, 0xa4200000, "Wii Nunchuk"),
	DEFINE_WII_PERIPHERAL(WII_CLASSIC_PRO, 0x0100a4200101, "Wii Classic Controller Pro / SNES controller"),
	DEFINE_WII_PERIPHERAL(WII_GUITAR, 0xa4200103, "Wii GH3 / GHWT Guitar")
};

/**
 * @brief Configure the i2c bus for communication with the wii peripheral.
 * 
 * @param dev : pointer to device driver
 * @retval 0 on success
 * @retval -errno otherwise
 */
static inline int wii_bus_config(const struct device *dev)
{
	const struct wii_periph_config *dev_cfg = dev->config;
	return i2c_configure(dev_cfg->i2c.bus, I2C_MODE_MASTER | I2C_SPEED_SET(dev_cfg->speed));
}

/**
 * @brief Perform a data read at the specified register (`reg`) with a delay
 * between the write and read commands. Official wii devices appear to have 
 * a latency between a request for data and the data actually becoming available.
 * As a result, it must be read with a delay.
 * 
 * @param i2c : i2c bus spec
 * @param reg : register to write before reading data back
 * @param data : buffer to place data into
 * @param len : length of data to read back
 * @retval 0 on success
 * @retval -errno otherwise
 */
static int wii_read_data_slow(const struct i2c_dt_spec * i2c, uint8_t reg, uint8_t * data, uint8_t len){
	int rc = 0;
	if ((rc = i2c_write_dt(i2c, &reg, sizeof(reg))) != 0){
			return rc;
	}
	/**
	 * @note busy waiting is preferable to sleeping in cases where
	 * we want to minimize the delay time. Sleeping resolves to ticks
	 * which are only accurate to 100us boundaries unless the tick rate
	 * is modified by the application. This comes at the cost of delaying
	 * scheduling of the idle thread. Perhaps it would be preferable to make
	 * this application configurable -- an option for immediate return, or an
	 * option for the return to be scheduled (and possible delayed).
	 * 
	 */
	k_busy_wait(CONFIG_WII_WRITE_READ_DELAY_US);
	return i2c_read_dt(i2c, data, 6);
}

/**
 * @brief Attempts to:
 * 	1. Change the data stream into an unencrypted format
 * 	2. Identify the specific peripheral attached, and update the device data accordingly
 * 
 * @param dev : pointer to device driver
 * @retval 0 on success
 * @retval -ENODEV if i2c was successful but not matching peripheral ID was found
 * @retval -errno otherwise
 */
static int handle_device_setup(const struct device *dev){
	const struct wii_periph_config *cfg = dev->config;
	struct wii_periph_data * data = dev->data;
	data->peripheral = NULL;
	int rc;
	const struct cmd_seq_item unecrypt_data_seq[] = {
		{0xf0, 0x55},
		{0xfb, 0x00}
	};
	/* Unencrypt Data */
	for (int i = 0; i < ARRAY_SIZE(unecrypt_data_seq); i++){
		const struct cmd_seq_item * pitem = &unecrypt_data_seq[i];
		rc = i2c_reg_write_byte_dt(&cfg->i2c, pitem->reg, pitem->data);
		if (rc != 0){
			return rc;
		}
		k_usleep(CONFIG_WII_INIT_SEQ_DELAY_US);
	}
	/* Identify Device */
	uint8_t buf[6] = {0};
	rc = wii_read_data_slow(&cfg->i2c, 0xfa, buf, 6);
	if (rc != 0){
		return rc;
	}
	k_usleep(CONFIG_WII_WRITE_READ_DELAY_US);

	/* Bit twiddling to have the ids match with simple comparison */
	uint64_t id = (*(uint64_t *)buf);
	
	for (int i = 0; i < ARRAY_SIZE(wii_peripheral_table); i++){
		const struct wii_peripheral * pitem = &wii_peripheral_table[i];
		if (id == pitem->id){
			LOG_DBG("Found: %s", pitem->label);
			data->peripheral = pitem;
			return 0;
		}
	}
	LOG_DBG("No Device Found");
	return -ENODEV;
}

/**
 * @brief Poll for the latest data frame. If an error occurs, 
 * it will treat this as a disconnection event, and will need
 * to reconfigure an attached device.
 * 
 * @param dev : pointer to device driver
 * @param wii : pointer to wii data frame where raw data will be placed
 * @retval 0 on success
 * @retval -ENOENT if no suitable device was found for polling data
 * @retval -errno otherwise
 */
static int wii_periph_poll_data(const struct device * dev, struct wii_btn_data * wii){
	if (wii == NULL){
		return -ENODEV;
	}
	struct wii_periph_data *data = dev->data;
	const struct wii_periph_config *cfg = dev->config;
	if(!data->peripheral){
		/* There is no supported peripheral attached */
		handle_device_setup(dev);
		if (!data->peripheral){
			/* Still no device -- don't fetch */
			return -ENOENT;
		}
	}

	int rc = wii_read_data_slow(&cfg->i2c, 0x00, wii->raw, 6);
	if (rc != 0){
		/* error reading device, assume a disconnect */
		data->peripheral = NULL;
	}
	return rc;
}

/**
 * @brief Initialize the driver. Will attempt to find an
 * attached controller right away.
 * 
 * @param dev : pointer to device driver
 * @retval 0 on success
 * @retval -errno otherwise
 */
static int wii_periph_init(const struct device *dev)
{
    struct wii_periph_data *data = dev->data;
	int rc = wii_bus_config(dev);
	if (rc == 0){
		handle_device_setup(dev);
		if (!data->peripheral){
			LOG_WRN("No supported device attached");
		}
	}
	return rc;
}

static struct wii_periph_data wii_periph_data = {
	.wii = {
		.raw = {0}
	},
	.peripheral = NULL
};

static const struct wii_periph_config wii_periph_cfg = {
	.i2c = I2C_DT_SPEC_INST_GET(0),
	.speed = I2C_SPEED_FAST
};

static const struct wii_periph_driver_api wii_api_funcs = {
	.fetch = wii_periph_poll_data,
};

DEVICE_DT_INST_DEFINE(0, wii_periph_init, NULL,
		    &wii_periph_data, &wii_periph_cfg, POST_KERNEL,
		    CONFIG_APPLICATION_INIT_PRIORITY, &wii_api_funcs);
