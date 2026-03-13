/*
 * ble.h
 *
 *  Created on: Nov 14, 2025
 *      Author: Bhakti Ramani
 */

#ifndef INC_BLE_H_
#define INC_BLE_H_

#include "app.h"

#include "sl_bluetooth.h"
#include "gatt_db.h"

typedef struct {
    // Common values
    bd_addr address;
    uint8_t address_type;

    // Server-specific values
    uint8_t advertisingSetHandle;
    uint8_t appConnectionHandle;     // Handle for the active connection
    bool connection_open;   //true when in open connection
    bool indication_enabled;  //true when client enabled indications
    bool indication_in_flight;  //true when n indication is in-flight
    bool ok_to_send_trackpad_reports;
    bool encrypted;
    bool bonding;
    bool bonded;

} ble_data_struct_t;

void handle_ble_event(sl_bt_msg_t *evt);

ble_data_struct_t* ble_get_data(void);

sl_status_t send_mouse_report(int16_t dx, int16_t dy, int8_t wheel, uint8_t buttons, ble_data_struct_t* ble_data);

sl_status_t send_single_tap(int16_t dx, int16_t dy, int8_t wheel, uint8_t buttons, ble_data_struct_t* ble_data);

sl_status_t send_xy(int16_t dx, int16_t dy, int8_t wheel, uint8_t buttons, ble_data_struct_t* ble_data);
#endif /* INC_BLE_H_ */
