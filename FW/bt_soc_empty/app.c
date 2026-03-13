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
trackpad_events event = IQS_RESET;
/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{

  iqs_error_e err;

  LOG_INFO("Intialization");
//  iqs5xx_init();
//  err = iqs_init();
//  if(err != IQS_OK)
//  {
//    LOG_INFO("Failed to initialize IQS driver: %d\n\r", err);
//    return;
//  }
//
//  err = iqs_end_of_communication();
//  if(err != IQS_OK)
//  {
//    LOG_INFO("Failed to complete end of communication IQS driver: %d\n\r", err);
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
static inline int8_t to_delta(uint8_t raw)
{
    return (int8_t)raw;  // reinterpret as signed
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
{
  sl_udelay_wait(10000);
  iqs_error_e err;

  iqs_full_events_t events;
  ble_data_struct_t *ble_data = ble_get_data();
  iqs_device_info_t info = {0};


  GPIO_PinOutSet(TEST_LED_PORT, 5);
  bool flag = gpio_irq_is_trackpad_ready();
//  sl_status_t sc;
if(flag)
{

          switch(event){
            case IQS_RESET :
              LOG_INFO(" #####################################################");
              LOG_INFO("=== Starting IQS5xx Initialization ===");
              LOG_INFO(" #####################################################");
              LOG_INFO("Step 1 : Reset...");

              err = iqs_reset();
              if (err != IQS_OK){
                  LOG_ERR("Reset failed : %s", iqs_error_to_str(err));
              }

              gpio_irq_clear_trackpad_ready();
              event = IQS_GET_DEVICE_INFO;

              break;

            case IQS_GET_DEVICE_INFO :
              LOG_INFO("Step 2: Check Setup Complete window and Reading device information...");
              iqs_check_setup_window();
              err = iqs_get_device_info(&info);
              if (err != IQS_OK){
                  LOG_ERR("Failed to get device info: %s", iqs_error_to_str(err));
              }
              LOG_INFO("Device Info - Product: 0x%04X, Project: 0x%04X", info.product_num, info.project_num);

              gpio_irq_clear_trackpad_ready();
              event = IQS_CHANNEL_CONFIG;
              break;

            case IQS_CHANNEL_CONFIG :
              LOG_INFO("Step 3 : Channel config ...");
              err = iqs_channel_config();

              if (err != IQS_OK){
                  LOG_ERR("Failed to write channel config : %s", iqs_error_to_str(err));
              }

              gpio_irq_clear_trackpad_ready();
              event = IQS_AUTO_ATI;
              break;


            case IQS_AUTO_ATI :
              LOG_INFO("Step 4: Performing Auto ATI...");
//              iqs_auto_ATI_force_high_ati_c();
//              iqs_read_ati_parameters();
//              iqs_read_all_count_values();
              err = iqs_auto_ATI();
//              iqs_check_setup_window();
//              iqs5xx_disable_alp();
//              iqs5xx_clear_alp_channels();
//              iqs5xx_manual_ati_config(500, 40, 128);
//              iqs_end_of_communication();

              iqs_read_all_count_values();
              iqs_read_ati_parameters();


              if(err != IQS_OK){
                  LOG_ERR("Failed to set AUTO ATI bit in system control 0 reg : %s", iqs_error_to_str(err));
              }

              gpio_irq_clear_trackpad_ready();
              event = IQS_ACK_RESET;
              break;

            case IQS_ACK_RESET :
              LOG_INFO("Step 5: Acknowledge Reset after Auto ATI ...");
              err = iqs_ack_reset();
              if (err != IQS_OK){
                  LOG_ERR("Initialization failed at reset acknowledgment: %s", iqs_error_to_str(err));
              }

              gpio_irq_clear_trackpad_ready();
              event = IQS_SYS_CONFIG;
              break;

            case IQS_SYS_CONFIG:
              LOG_INFO("Step 6 : Set events (setup complete) & threshold check, channel config");

              err = iqs_sys_config_fixed();
              if(err != IQS_OK){
                 LOG_ERR("Failed to set events in system config register : %s", iqs_error_to_str(err));
              }

              err = iqs_check_threshold();
              if(err != IQS_OK){
                 LOG_ERR("Failed to check threshold register : %s", iqs_error_to_str(err));
              }

              gpio_irq_clear_trackpad_ready();
              event = IQS_TEST_CHANNELS1;
              break;


            case IQS_TEST_CHANNELS1 :
              LOG_INFO("Step 7 : Channel Test 1 ");
//              iqs_read_all_delta_values();
//              iqs_read_all_count_values();

              gpio_irq_clear_trackpad_ready();
              event = IQS_TEST_CHANNELS2;
              break;

            case IQS_TEST_CHANNELS2 :
              LOG_INFO("Step 8 : Channel Test 2");

//              iqs_read_channels_table();
//              iqs_read_active_channels();

              gpio_irq_clear_trackpad_ready();
              event = IQS_READ_EVENTS;
              break;

          case IQS_READ_EVENTS :


//              LOG_INFO("=========================================================================================Touch EVENT");
//
//
//              iqs_read_sys_flags();
//
//              err = iqs_read_all_events(&events);
//              if(err != IQS_OK)
//              {
//                  GPIO_PinOutSet(TEST_LED_PORT, TEST_LED_PIN_R);
//
//              }
//
//              err = iqs_end_of_communication();
//              if(err != IQS_OK)
//              {
//                  GPIO_PinOutSet(TEST_LED_PORT, TEST_LED_PIN_R);
//
//              }
//
//              int16_t dx = (int16_t)events.rel_x;
//              int16_t dy = (int16_t)events.rel_y;
//    //
//    //          if(events.single_tap){
//    //              send_single_tap(dx, dy, 0, 0, ble_data);
//    //          }else{
//    //              send_xy(dx, dy, 0, 0, ble_data);
//    //          }
//              LOG_INFO("Sent mouse: dx=%d, dy=%d", dx, dy);

              gpio_irq_clear_trackpad_ready();


           break;

          }

}
else{
    GPIO_PinOutClear(TEST_LED_PORT, TEST_LED_PIN_Y);
}

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
