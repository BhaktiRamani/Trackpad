  if(flag)
  {
      switch(event){
        case IQS_RESET :
          gpio_irq_clear_trackpad_ready();
          LOG_INFO("=== Starting IQS5xx Initialization ===");
          LOG_INFO("Step 1: Acknowledging reset...");

          err = iqs_ack_reset();
          if (err != IQS_OK){
              LOG_ERR("Reset failed : %s", iqs_error_to_str(err));
          }
          event = IQS_GET_DEVICE_INFO;
          sl_udelay_wait(5000);  // 5ms
          break;

        case IQS_GET_DEVICE_INFO :
          gpio_irq_clear_trackpad_ready();
          LOG_INFO("Step 2: Reading device information...");
          err = iqs_get_device_info(&info);
          if (err != IQS_OK){
              LOG_ERR("Failed to get device info: %s", iqs_error_to_str(err));
          }
          LOG_INFO("Device Info - Product: 0x%04X, Project: 0x%04X", info.product_num, info.project_num);
          sl_udelay_wait(5000);  // 5ms
          event = IQS_AUTO_ATI;
          break;

        case IQS_AUTO_ATI :
          gpio_irq_clear_trackpad_ready();
          LOG_INFO("Step 3: Performing Auto ATI...");
          err = iqs_auto_ATI();
          if(err != IQS_OK){
              LOG_ERR("Failed to set AUTO ATI bit in system control 0 reg : %s", iqs_error_to_str(err));
          }
          sl_udelay_wait(5000);  // 5ms
          event = IQS_ACK_RESET;
          break;

        case IQS_ACK_RESET :
          LOG_INFO("Step 4: Acknowledge Reset after Auto ATI ...");
          err = iqs_ack_reset();
          if (err != IQS_OK){
              LOG_ERR("Initialization failed at reset acknowledgment: %s", iqs_error_to_str(err));
          }
          sl_udelay_wait(5000);  // 5ms
          event = IQS_GESTURE_DISABLE;
          break;

        case IQS_GESTURE_DISABLE :
          gpio_irq_clear_trackpad_ready();
          LOG_INFO("Step 5 : Disable Gestures in gesture enable register");
          err = iqs_gesture_disable();
          if(err != IQS_OK){
              LOG_ERR("Failed to disable gestures in gesture enable register : %s", iqs_error_to_str(err));
          }
          sl_udelay_wait(5000);  // 5ms
          event = IQS_SYS_CONFIG;
         break;

          case IQS_SYS_CONFIG:
            LOG_INFO("Step 6 : Set events (event mode | prox mode | gesture mode");
            gpio_irq_clear_trackpad_ready();
            iqs_set_total_tx_rx();

           err = iqs_sys_config();
           if(err != IQS_OK){
               LOG_ERR("Failed to set events in system config register : %s", iqs_error_to_str(err));
           }
           sl_udelay_wait(5000);  // 5ms
            event = IQS_READ_EVENTS;
            break;



          case IQS_GESTTURE_ENABLE :
            gpio_irq_clear_trackpad_ready();
            LOG_INFO("Step 7 : Enable Gestures in gesture enable register");
            err = iqs_gesture_enable();
            if(err != IQS_OK){
                LOG_ERR("Failed to enable gestures in gesture enable register : %s", iqs_error_to_str(err));
            }
            uint8_t delta_values[IQS_DELTA_LENGTH];

            iqs_error_e err = iqs_read_delta_values(delta_values);

            if (err == IQS_OK) {
                LOG_INFO("Delta values read successfully.");
                for (int tx = 0; tx < 30; tx++)
                {
                    printf("TX%02d: ", tx);
                    for (int rx = 0; rx < 10; rx++)
                    {
                        int idx = tx * 10 + rx;
                        int8_t d = to_delta(delta_values[idx]);
                        printf("%4d", d);
                    }
                    printf("\n\r");
                }
            } else {
                LOG_ERR("Error reading delta values: %s", iqs_error_to_str(err));
            }
            event = IQS_READ_EVENTS;
            break;

      case IQS_READ_EVENTS :


          LOG_INFO("=========================================================================================Touch EVENT");


          iqs_read_sys_flags();

          err = iqs_read_all_events(&events);
          if(err != IQS_OK)
          {
              GPIO_PinOutSet(TEST_LED_PORT, TEST_LED_PIN_R);

          }

          err = iqs_end_of_communication();
          if(err != IQS_OK)
          {
              GPIO_PinOutSet(TEST_LED_PORT, TEST_LED_PIN_R);

          }

          int16_t dx = (int16_t)events.rel_x;
          int16_t dy = (int16_t)events.rel_y;
//
//          if(events.single_tap){
//              send_single_tap(dx, dy, 0, 0, ble_data);
//          }else{
//              send_xy(dx, dy, 0, 0, ble_data);
//          }
          LOG_INFO("Sent mouse: dx=%d, dy=%d", dx, dy);

          gpio_irq_clear_trackpad_ready();


       break;

      }
}