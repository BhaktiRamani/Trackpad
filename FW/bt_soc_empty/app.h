/***************************************************************************//**
 * @file
 * @brief Application interface provided to main().
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

#ifndef APP_H
#define APP_H

#include "inc/i2c.h"
#include "inc/iqs_headers.h"
#include "inc/iqs572.h"
#include "inc/irq.h"
#include "inc/gpio.h"
#include "inc/drv2605.h"
#include "inc/pwm.h"
#include "inc/hid.h"
#include "inc/ble.h"
#include "inc/trackpad_hid.h"
#include "inc/iqs550_test_func.h"
#include "inc/iqs550_new.h"


#include "stdbool.h"
#include "stddef.h"
#include "string.h"
#include "em_cmu.h"
#include "sl_udelay.h"


typedef enum {
  IQS_RESET,
  IQS_GET_DEVICE_INFO,
  IQS_CHANNEL_CONFIG,
  IQS_AUTO_ATI,
  IQS_ACK_RESET,
  IQS_SYS_CONFIG,
  IQS_TEST_CHANNELS1,
  IQS_TEST_CHANNELS2,
  IQS_GESTTURE_ENABLE,
  IQS_GESTURE_DISABLE,
  IQS_READ_EVENTS,
  None
}trackpad_events;

#define TRACKPAD_MCU_RDY_PORT  gpioPortC
#define TRACKPAD_MCU_RDY_PIN   9
#define TRACKPAD_MCU_RDY_MASK (1<<9)
#define TRACKPAD_RDY_INT       TRACKPAD_MCU_RDY_PIN
#define TRACKPAD_RDY_INT_EVEN  ((TRACKPAD_MCU_RDY_PIN % 2) == 0)

#define HAPTICS_EN_PORT gpioPortC
#define HAPTICS_EN_PIN  7

#define HAPTICS_INT_PORT  gpioPortC
#define HAPTICS_INT_PIN   8

#define LED_DI_PORT gpioPortA
#define LED_DI_PIN  0

#define DEFAULT_I2C_BUS I2C0


// ANSI color codes
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define RESET   "\033[0m"

// Colored tag and function name only — message stays white
#define LOG_ERR(fmt, ...) \
    sl_iostream_printf(SL_IOSTREAM_STDOUT, RED   "[ERR]  [%s]" RESET " " fmt "\n\r", __func__, ##__VA_ARGS__)

#define LOG_WARN(fmt, ...) \
    sl_iostream_printf(SL_IOSTREAM_STDOUT, YELLOW"[WARN] [%s]" RESET " " fmt "\n\r", __func__, ##__VA_ARGS__)

#define LOG_INFO(fmt, ...) \
    sl_iostream_printf(SL_IOSTREAM_STDOUT, GREEN "[INFO] [%s]" RESET " " fmt "\n\r", __func__, ##__VA_ARGS__)


#define LOG_DBG(fmt, ...)  sl_iostream_printf(SL_IOSTREAM_STDOUT, "[DEBUG] " fmt "\n\r", ##__VA_ARGS__)


//#define ENABLE_DEBUG_LOGS
//
//#ifdef ENABLE_DEBUG_LOGS
//  #define LOG_INFO(fmt, ...) ...
//#else
//  #define LOG_INFO(fmt, ...)
//#endif

//
//#define __BYTE_ORDER__ = 0
//// Detect system endianness
//#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
//  #define SYSTEM_LITTLE_ENDIAN 1
//#else
//  #define SYSTEM_LITTLE_ENDIAN 0
//#endif

// Convert 16-bit value to Big Endian (MSB first)
#define TO_BIG_ENDIAN_16(val)  ((uint16_t)((((val) >> 8) & 0xFF) | (((val) & 0xFF) << 8)))

//// Convert 16-bit value to Little Endian (LSB first)
//#define TO_LITTLE_ENDIAN_16(val) (val)  // already little-endian on most MCUs

// Get bytes from 16-bit value in desired endianness
#define GET_BYTE_HIGH(val)  ((uint8_t)(((val) >> 8) & 0xFF))
#define GET_BYTE_LOW(val)   ((uint8_t)((val) & 0xFF))

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void);

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
void app_process_action(void);

#endif // APP_H
