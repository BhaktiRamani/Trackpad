#include "../inc/trackpad_hid.h"

/**
 * Send mouse HID report with relative movement
 */
//sl_status_t send_mouse_report(int16_t dx, int16_t dy, int8_t wheel, uint8_t buttons, ble_data_struct_t* ble_data)
//{
//    sl_status_t sc;
//    sMouseReport_t report;
//
//    // Check if ready to send
//    if (!ble_data -> connection_open || !ble_data -> ok_to_send_trackpad_reports)
//    {
//        return SL_STATUS_INVALID_STATE;
//    }
//
//    // Fill the mouse report
//    report.reportID = 1;           // Mouse report ID
//    report.buttonMask = buttons;    // Button states (bit 0=left, bit 1=right, etc.)
//    report.dx = dx;                 // X movement (-127 to 127)
//    report.dy = dy;                 // Y movement (-127 to 127)
//    report.dWheel = wheel;          // Wheel movement
//
//    // Send notification
//    sc = sl_bt_gatt_server_send_notification(
//        ble_data -> appConnectionHandle,
//        gattdb_human_interface_device,      // Your HID Report characteristic handle
//        sizeof(sMouseReport_t),
//        (uint8_t*)&report);
//
//    if (sc != SL_STATUS_OK)
//    {
//        LOG_ERR("Mouse report failed: 0x%04x", (unsigned int)sc);
//    }
//
//    return sc;
//}
