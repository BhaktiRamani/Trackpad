/***************************************************************************//**
 * @file
 * @brief Core application logic.
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
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "app.h"
#include "stdio.h"

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{

//  iqs_error_e err;
//
//  LOG_INFO("Initialization");
//  err = iqs_init();
//  if(err != IQS_OK)
//  {
//    LOG_INFO(
//                      "Failed to initialize IQS driver: %d\n\r", err);
//    return;
//  }
//
//  err = iqs_end_of_communication();
//  if(err != IQS_OK)
//  {
//    LOG_INFO(
//                      "Failed to complete end of communication IQS driver: %d\n\r", err);
//    return;
//  }

  gpio_init();

//  init_pwm(WS2812_FREQ);
//  eLibrarySelect lib = B;
//  eWaveform wf = {
//    .wait = 0,
//    .waveform = 1
//  };
//  init_drv2605(lib, &wf, 1);
//  load_play_drv2605(lib, &wf, 1, true);

}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
{

//  iqs_error_e err;
////  iqs_coordinates_t coords;
//  iqs_full_events_t events;
//  ble_data_struct_t *ble_data = ble_get_data();
//  // wait for an event
//  GPIO_PinOutClear(TEST_LED_PORT, TEST_LED_PIN_Y);
//  bool flag = gpio_irq_is_trackpad_ready();
//  if(flag)
//  {
//      LOG_INFO("");
//      LOG_INFO("=========================================================================================PROX EVENT");
//      gpio_irq_clear_trackpad_ready();
//      GPIO_PinOutSet(TEST_LED_PORT, TEST_LED_PIN_Y);
//
//      // Verify configuration was written correctly
//      iqs_read_sys_flags();
//
//      // read events
//      err = iqs_read_all_events(&events);
//      if(err != IQS_OK)
//      {
          GPIO_PinOutSet(TEST_LED_PORT, 4);
//
//      }
//
//      // end the communication
//      err = iqs_end_of_communication();
//      if(err != IQS_OK)
//      {
//          GPIO_PinOutSet(TEST_LED_PORT, TEST_LED_PIN_R);
//
//      }
//
//      // Send mouse report with trackpad data
//      int16_t dx = (int16_t)events.rel_x;  // Cast to int8_t (adjust if needed)
//      int16_t dy = (int16_t)events.rel_y;
//      uint8_t buttons = 0;  // Extract from events if available
//
//      send_mouse_report(dx, dy, 0, buttons, ble_data);
//
//     LOG_INFO("Sent mouse: dx=%d, dy=%d, btn=0x%02x", dx, dy, buttons);
//
//  }
//  else{
//      GPIO_PinOutClear(TEST_LED_PORT, TEST_LED_PIN_Y);
//  }

//  send_data(0);      // Green = 0
//    send_data(255);    // Red = 255
//    send_data(0);      // Blue = 0
//    duty_pwm(0);
//
//    // Delay 1 second to observe
//    for (volatile uint32_t i = 0; i < 3840000; i++);
//
//    // Test 2: Pure Green
//    send_data(255);    // Green = 255
//    send_data(0);      // Red = 0
//    send_data(0);      // Blue = 0
//    duty_pwm(0);
//
//    // Delay 1 second
//    for (volatile uint32_t i = 0; i < 3840000; i++);
//
//    // Test 3: Pure Blue
//    send_data(0);      // Green = 0
//    send_data(0);      // Red = 0
//    send_data(255);    // Blue = 255
//    duty_pwm(0);

}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  handle_ble_event(evt);
}
