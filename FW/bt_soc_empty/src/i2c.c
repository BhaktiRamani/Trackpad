/*
 * i2c.c
 *
 *  Created on: Oct 8, 2025
 *      Author: Bhakti Ramani
 */


#include "app.h"
#include "../inc/i2c.h"

/**
 * @brief 
 * 
 * @param addr Should be 7 bit non shifted address
 * @param subaddr sub address to read from 
 * @param rx_buffer  read buffer
 * @param rx_size  read len
 */
//S+ADDR(R)+DATA0+P
I2C_TransferReturn_TypeDef i2c_read(uint8_t addr, uint16_t reg_addr,
                                     uint8_t *data, size_t len)
{
  while (GPIO_PinInGet(TRACKPAD_MCU_RDY_PORT, TRACKPAD_MCU_RDY_PIN) == 0)
  {
      // Wait for RDY to go LOW
  }

  I2C_TransferSeq_TypeDef i2cTransfer;
  I2C_TransferReturn_TypeDef status;

  // Prepare 16-bit register address (big-endian)
  uint8_t reg_addr_buf[2];
  reg_addr_buf[0] = (reg_addr >> 8) & 0xFF;  // High byte
  reg_addr_buf[1] = reg_addr & 0xFF;         // Low byte

  i2cTransfer.addr  = (addr << 1);           // Shift left (7-bit addr)
  i2cTransfer.flags = I2C_FLAG_WRITE_READ;

  // Write register address
  i2cTransfer.buf[0].data = reg_addr_buf;
  i2cTransfer.buf[0].len  = 2;

  // Read data
  i2cTransfer.buf[1].data = data;
  i2cTransfer.buf[1].len  = len;

  status = I2C_TransferInit(DEFAULT_I2C_BUS, &i2cTransfer);
  while (status == i2cTransferInProgress)
  {
    status = I2C_Transfer(DEFAULT_I2C_BUS);
  }

  return status;

}

// Here send reg_addr + data as one array, some issue in driver, it is not sending .buf[1].data and buf[1].len
I2C_TransferReturn_TypeDef i2c_write(uint8_t addr, uint16_t reg_addr,
                                        uint8_t *data, size_t len)
{
  while (GPIO_PinInGet(TRACKPAD_MCU_RDY_PORT, TRACKPAD_MCU_RDY_PIN) == 0)
  {
      // Wait for RDY to go LOW
  }

  I2C_TransferSeq_TypeDef i2cTransfer;
  I2C_TransferReturn_TypeDef status;

  // Allocate buffer for register address + data
  // Note: This uses stack allocation. For large writes, consider static buffer
  uint8_t tx_buf[2 + len];

  // Pack register address (big-endian) and data
  tx_buf[0] = (reg_addr >> 8) & 0xFF;  // High byte
  tx_buf[1] = reg_addr & 0xFF;         // Low byte

  // Copy data after register address
  if (data != NULL && len > 0) {
    memcpy(&tx_buf[2], data, len);
  }

  i2cTransfer.addr  = (addr << 1);     // Shift left (7-bit addr)
  i2cTransfer.flags = I2C_FLAG_WRITE;

  // Send register address + data as single buffer
  i2cTransfer.buf[0].data = tx_buf;
  i2cTransfer.buf[0].len  = 2 + len;

  status = I2C_TransferInit(DEFAULT_I2C_BUS, &i2cTransfer);
  while (status == i2cTransferInProgress)
  {
    status = I2C_Transfer(DEFAULT_I2C_BUS);
  }

  return status;
}










































