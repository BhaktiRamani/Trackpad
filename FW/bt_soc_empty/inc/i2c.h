/*
 * i2c.h
 *
 *  Created on: Oct 8, 2025
 *      Author: Bhakti Ramani
 */

#ifndef HEADER_I2C_H_
#define HEADER_I2C_H_

#include "em_device.h"
#include "em_chip.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_i2c.h"
#include "stddef.h"


#define IQS_I2C_WRITE_ADDR 0xE8
#define IQS_I2C_READ_ADDR 0xE9
#define IQS_I2C_ADDR 0x74  // 0xE8 >> 1

#define LED_PORT     gpioPortF
#define LED_PIN      4

void i2c_init();

I2C_TransferReturn_TypeDef i2c_read(uint8_t addr, uint16_t reg_addr,
                                     uint8_t *data, size_t len);

I2C_TransferReturn_TypeDef i2c_write(uint8_t addr, uint16_t reg_addr,
                                        uint8_t *data, size_t len);

#endif /* HEADER_I2C_H_ */
