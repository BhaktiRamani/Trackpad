/*
 * ble.c
 *
 *  Created on: Nov 14, 2025
 *      Author: Bhakti Ramani
 */


#include "../inc/ble.h"

ble_data_struct_t ble_data;
// The advertising set handle allocated from Bluetooth stack.
//static uint8_t advertising_set_handle = 0xff;


ble_data_struct_t* ble_get_data(void){
  return (&ble_data);
}

// Advertising interval for BLE in units of 0.625ms.
// ADV_INTERVAL of 400 translates to 250ms (400 * 0.625ms = 250ms).
#define ADV_INTERVAL 160

// Connection interval for BLE in units of 1.25ms.
// CON_INTERVAL of 60 translates to 75ms (60 * 1.25ms = 75ms).
#define CON_INTERVAL 8

// Connection latency for BLE.
// Defines the number of connection events the slave can skip.
// CON_LATENCY of 4 means the slave device can skip up to 4 connection intervals.
#define CON_LATENCY 0

// Supervision timeout for BLE in units of 10ms.
// CON_TIMEOUT of 400 translates to 4 seconds (76 * 10ms = 760ms > 750ms).
// #define CON_TIMEOUT ((1 + CON_LATENCY) * CON_INTERVAL * 2) = 750ms
#define CON_TIMEOUT 100

// Maximum length of the connection event in BLE.
// MAX_CE_LEN set to 0xFFFF indicates no specific maximum length,
// allowing for the longest possible connection events.
#define MAX_CE_LEN 0xFFFF

uint8_t system_id[8]; // Buffer for the System ID value

void handle_ble_event(sl_bt_msg_t *evt){
  sl_status_t sc;

  switch (SL_BT_MSG_ID(evt->header)) {

    case sl_bt_evt_system_boot_id:

        // Initialize BLE stack
        sc = sl_bt_system_get_identity_address(&ble_data.address, &ble_data.address_type);
        if (sc != SL_STATUS_OK)
        {
            LOG_ERR("status=0x%04x", (unsigned int)sc);
        }

        // The System ID is derived from the device's Bluetooth address with a standard format
        system_id[0] = ble_data.address.addr[5];
        system_id[1] = ble_data.address.addr[4];
        system_id[2] = ble_data.address.addr[3];
        system_id[3] = 0xFF; // Middle bytes are fixed according to the Bluetooth SIG's specification
        system_id[4] = 0xFE; // Middle bytes are fixed according to the Bluetooth SIG's specification
        system_id[5] = ble_data.address.addr[2];
        system_id[6] = ble_data.address.addr[1];
        system_id[7] = ble_data.address.addr[0];


        // Write the System ID to the GATT database
        sc = sl_bt_gatt_server_write_attribute_value(gattdb_system_id,
                                                     0,                 // Attribute value offset
                                                     sizeof(system_id), // Length of the System ID
                                                     system_id);        // System ID value
        if (sc != SL_STATUS_OK)
        {
            LOG_ERR("status=0x%04x", (unsigned int)sc);
        }

        // Create advertising set
        sc = sl_bt_advertiser_create_set(&ble_data.advertisingSetHandle);
        if (sc != SL_STATUS_OK) {
            LOG_ERR("advertiser_create_set failed: 0x%04x", (unsigned int)sc);
        }

        sc = sl_bt_legacy_advertiser_generate_data(ble_data.advertisingSetHandle,
                                                   sl_bt_advertiser_general_discoverable);
        if (sc != SL_STATUS_OK) {
            LOG_ERR("set_data failed: 0x%04x", (unsigned int)sc);
        }

        // Set advertising timing (faster for HID discovery)
        sc = sl_bt_advertiser_set_timing(
            ble_data.advertisingSetHandle,
            ADV_INTERVAL,         // Min interval: 20ms (32 * 0.625ms)
            ADV_INTERVAL,         // Max interval: 20ms
            0,          // Duration: 0 = advertise indefinitely
            0);         // Max events: 0 = no limit
        if (sc != SL_STATUS_OK) {
            LOG_ERR("set_timing failed: 0x%04x", (unsigned int)sc);
        }

        sc = sl_bt_sm_delete_bondings();
        if (sc != SL_STATUS_OK)
            LOG_ERR("delete_bondings failed: 0x%04x", sc);

        sc = sl_bt_sm_configure(0x0B, sl_bt_sm_io_capability_noinputnooutput);
        if (sc != SL_STATUS_OK) {
            LOG_ERR("Bt_sm_configure failed: 0x%04x", (unsigned int)sc);
        }

        sc = sl_bt_sm_set_bondable_mode(1);
        if (sc != SL_STATUS_OK) {
            LOG_ERR("sl_bt_sm_set_bondable_mode failed: 0x%04x", (unsigned int)sc);
        }

        // Store bonding configuration - FIXED parameters
        sc = sl_bt_sm_store_bonding_configuration(
            8,   // Max bonding count (allow multiple devices)
            0x02); // Policy: overflow oldest bondings
        if (sc != SL_STATUS_OK) {
            LOG_ERR("sl_bt_sm_store_bonding_configuration failed: 0x%04x", (unsigned int)sc);
        }


        // Start advertising (connectable and scannable)
        sc = sl_bt_legacy_advertiser_start(
            ble_data.advertisingSetHandle,
            sl_bt_advertiser_connectable_scannable);
        if (sc != SL_STATUS_OK) {
            LOG_ERR("start advertising failed: 0x%04x", (unsigned int)sc);
        }
        LOG_INFO("Advertising started with HID service UUID");
        break;

    case sl_bt_evt_connection_opened_id :
        ble_data.appConnectionHandle = evt->data.evt_connection_opened.connection;
        ble_data.connection_open = true;
        sc = sl_bt_connection_set_parameters(evt->data.evt_connection_opened.connection,
                                        CON_INTERVAL,
                                        CON_INTERVAL,
                                        CON_LATENCY,
                                        CON_TIMEOUT,
                                        0,
                                        MAX_CE_LEN);
        if (sc != SL_STATUS_OK)
            LOG_ERR("conn params failed: 0x%04x", sc);

        LOG_INFO("Connection Open, Advertisement stopped");

        // ---- Request security (pairing + bonding) ----
        sc = sl_bt_sm_increase_security(ble_data.appConnectionHandle);
        if (sc != SL_STATUS_OK)
            LOG_ERR("increase_security failed: 0x%04x", sc);
        else
            LOG_INFO("Security request sent");

        break;

    case sl_bt_evt_sm_confirm_passkey_id:
      ble_data.bonding = true;
      ble_data.bonded = false;
      ble_data.encrypted = false;
      LOG_INFO("Confirm passkey ID : %d", evt -> data.evt_sm_passkey_display.passkey);

//       Auto-confirm passkey (since we're using no input/output)
      sc = sl_bt_sm_passkey_confirm(ble_data.appConnectionHandle, 1);
      if (sc != SL_STATUS_OK)
          LOG_ERR("passkey_confirm failed: 0x%04x", sc);

      break;

    case sl_bt_evt_sm_confirm_bonding_id:
        LOG_INFO("Bonding started");
        sc = sl_bt_sm_bonding_confirm(ble_data.appConnectionHandle, 1);
        if (sc != SL_STATUS_OK)
            LOG_ERR("Bonding confirm failed: 0x%04x", sc);

        break;

    case sl_bt_evt_sm_bonded_id:
        LOG_INFO("Bonding success");
        ble_data.encrypted = true;
        ble_data.bonding = false;
        ble_data.bonded = true;
        break;

    case sl_bt_evt_sm_bonding_failed_id:
        LOG_ERR("Bonding failed, reason: 0x%04x",
                evt->data.evt_sm_bonding_failed.reason);
        ble_data.bonding = false;
        ble_data.bonded = false;
        ble_data.encrypted = false;
        // Close connection on bonding failure
        sc = sl_bt_connection_close(ble_data.appConnectionHandle);
        if (sc != SL_STATUS_OK)
            LOG_ERR("connection_close failed: 0x%04x", sc);
        break;



    case sl_bt_evt_connection_parameters_id:
        // Check if encrypted
        if (evt->data.evt_connection_parameters.security_mode > 1) {
            ble_data.encrypted = true;
            LOG_INFO("Link encrypted");
        }
        break;

    case sl_bt_evt_system_external_signal_id :
      LOG_INFO("External Event Triggered");
      if ((evt->data.evt_system_external_signal.extsignals == 1) &&
          (ble_data.ok_to_send_trackpad_reports == true)) {
          sMouseReport_t tmp;
           tmp.buttonMask = 0;
           tmp.dx = 25;
           tmp.dy = 25;
           sc = sl_bt_gatt_server_send_notification(
               ble_data.appConnectionHandle,  // Connection handle
               gattdb_report,
               sizeof(sMouseReport_t),
               (uint8_t*)&tmp);  // Cast struct pointer to uint8_t*
          if (sc != SL_STATUS_OK){
              LOG_ERR("Mouse report failed: 0x%04x", sc);
          }
          else{
              LOG_INFO("Link encrypted");
          }
      }
      break;


    case sl_bt_evt_connection_closed_id:
         ble_data.ok_to_send_trackpad_reports = false;
        sc = sl_bt_legacy_advertiser_start(ble_data.advertisingSetHandle,sl_bt_advertiser_connectable_scannable );
        if (sc != SL_STATUS_OK)
        {
            LOG_ERR("status=0x%04x", (unsigned int)sc);
        }

        ble_data.connection_open = false;
        ble_data.indication_enabled = false;
        ble_data.indication_in_flight = false;
        ble_data.bonded = false;
        ble_data.bonding = false;
        ble_data.encrypted = false;
        ble_data.appConnectionHandle = 0;            // Reset the connection handle
        LOG_INFO("Connection Close, Advertisement started again");
        break;

    case sl_bt_evt_gatt_server_characteristic_status_id:
        // Check if this is for the HID Report characteristic
      LOG_INFO("Generated characterisitc : evt->data.evt_gatt_server_characteristic_status.characteristic : %d\n", evt->data.evt_gatt_server_characteristic_status.characteristic);

        if (evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_report)
        {
            LOG_INFO("Level 2");
            // Check the status type
            if (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_client_config)
            {
                LOG_INFO("Level 3");
                // Check if notifications have been enabled
                if (evt->data.evt_gatt_server_characteristic_status.client_config_flags & sl_bt_gatt_server_notification)
                {
                    LOG_INFO("Level 4");
                    // Notifications enabled - ready to send HID reports
                    ble_data.ok_to_send_trackpad_reports = true;
                    LOG_INFO("HID notifications enabled");
                }
                else
                {
                    // Notifications disabled
                    ble_data.ok_to_send_trackpad_reports = false;
                    LOG_INFO("HID notifications disabled");
                }
            }
        }
        break;

    case sl_bt_evt_gatt_server_indication_timeout_id :
      ble_data.ok_to_send_trackpad_reports = false;
      break;

    default:
      break;
  }

}
static int16_t prev_x = 0;
static int16_t prev_y = 0;
static bool first_reading = true;


// Function to convert 16-bit absolute to 8-bit relative
int8_t calculate_relative_movement(uint16_t current_pos, int16_t *prev_pos, bool *first)
{
    if (*first) {
        // First reading - no movement yet
        *prev_pos = current_pos;
        return 0;
    }

    // Calculate delta (difference from previous position)
    int32_t delta = (int32_t)current_pos - (int32_t)*prev_pos;

    // Handle wraparound (if sensor goes from 65535 to 0)
    if (delta > 32768) {
        delta -= 65536;
    } else if (delta < -32768) {
        delta += 65536;
    }

    // Clamp to 8-bit range (-127 to +127)
    if (delta > 127) {
        delta = 127;
    } else if (delta < -127) {
        delta = -127;
    }

    // Update previous position
    *prev_pos = current_pos;

    return (int8_t)delta;
}


sl_status_t send_mouse_report(int16_t dx, int16_t dy, int8_t wheel, uint8_t buttons, ble_data_struct_t* ble_data)
{
    sl_status_t sc;
    sMouseReport_t report;

    if (!ble_data -> connection_open || !ble_data -> ok_to_send_trackpad_reports)
    {
        return SL_STATUS_INVALID_STATE;
    }

    // Convert to relative movement
     int8_t relative_x = calculate_relative_movement(dx, &prev_x, &first_reading);
     int8_t relative_y = calculate_relative_movement(dy, &prev_y, &first_reading);

     first_reading = false;

     report.buttonMask = buttons;
     report.dx = relative_x;
     report.dy = relative_y;

    // Send notification
    sc = sl_bt_gatt_server_send_notification(
        ble_data -> appConnectionHandle,
        gattdb_report,
        sizeof(sMouseReport_t),
        (uint8_t*)&report);

    if (sc != SL_STATUS_OK)
    {
        LOG_ERR("Mouse report failed: 0x%04x", (unsigned int)sc);
        return sc;
    }
    LOG_INFO("Mouse report successfull");

    return sc;
}

sl_status_t send_single_tap(int16_t dx, int16_t dy, int8_t wheel, uint8_t buttons, ble_data_struct_t* ble_data)
{
  sl_status_t sc;
  uint8_t button = 0x01;
  sc = send_mouse_report(dx, dy, 0, button, ble_data);
  if(sc != SL_STATUS_OK  )
  {
        LOG_ERR("Mouse report failed: 0x%04x", (unsigned int)sc);
        return sc;
  }

  sl_sleeptimer_delay_millisecond(50);
  button = 0x00;
  sc = send_mouse_report(dx, dy, 0, button, ble_data);
  if(sc != SL_STATUS_OK  )
  {
       LOG_ERR("Mouse report failed: 0x%04x", (unsigned int)sc);
       return sc;
  }
  return sc;
}

sl_status_t send_xy(int16_t dx, int16_t dy, int8_t wheel, uint8_t buttons, ble_data_struct_t* ble_data)
{
  sl_status_t sc;
  sc = send_mouse_report(dx, dy, 0, buttons, ble_data);
  if(sc != SL_STATUS_OK  )
  {
       LOG_ERR("Mouse report failed: 0x%04x", (unsigned int)sc);
       return sc;
  }
  return sc;
}



