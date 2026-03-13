/*
 * iqs572.h
 *
 *  Created on: Oct 30, 2025
 *      Author: Bhakti Ramani
 */

#ifndef INC_IQS572_H_
#define INC_IQS572_H_

#include "app.h"

typedef enum iqs_info{
  fail = -1,
  project_num = 15,
  product_num_550 = 40,
  product_num_572 = 58,

}iqs_info_e;




// Channel configuration for custom PCB
typedef struct {
    uint8_t total_rx;               // Number of Rx channels (10 for your PCB)
    uint8_t total_tx;               // Number of Tx channels (15 for your PCB)
    uint8_t rx_mapping[10];         // Rx mapping table
    uint8_t tx_mapping[15];         // Tx mapping table
    uint16_t x_resolution;          // X resolution
    uint16_t y_resolution;          // Y resolution
    uint8_t xy_config;              // Axis flip/swap configuration
} iqs_channel_config_t;

typedef struct{
  uint8_t project_num;
  uint8_t product_num;
  uint8_t major_ver;
  uint8_t minor_ver;
}iqs_device_info_t;

typedef struct{
  uint16_t x;
  uint16_t y;
}iqs_coordinates_t;


typedef struct{
    uint8_t gesture_events_0;
    uint8_t gesture_events_1;
    uint8_t system_info_0;
    uint8_t system_info_1;
    uint8_t num_fingers;
    uint16_t rel_x;
    uint16_t rel_y;
    uint16_t abs_x;
    uint16_t abs_y;
    bool single_tap;
    bool press_and_hold;
    bool double_tap;
    bool scroll;
    bool zoom;
}iqs_full_events_t;





/** IQS I2C Device address **/
#define IQS_I2C_ADDR 0x74  // 0xE8 >> 1


/** Register Address **/
#define IQS_REG_DEVICE_INFO             0x00
#define IQS_REG_X_COORD                 0x12
#define IQS_RED_Y_COORD                 0x14
#define IQS_END_OF_COMMUNICATION        0xEEEE

#define IQS_REG_SYS_CONFIG_1            0x058F
#define IQS_REG_SYS_CONFIG_0            0x058E

#define IQS_REG_SYSTEM_INFO_0           0x000F
#define IQS_REG_SYSTEM_INFO_1           0x0010

#define IQS_REG_SYS_CONTROL_1           0x0432
#define IQS_REG_SYS_CONTROL_0           0x0431

#define IQS_REG_GESTURE_EVENTS_0        0x000D

#define IQS_REG_SINGLE_FINGER_GESTURE_EN  0x06B7
#define IQS_REG_MULTI_FINGER_GESTURE_EN   0x06B8

#define IQS_REG_TOTAL_RX      0x063D
#define IQS_REG_TOTAL_TX      0x063E

#define IQS_REG_RX_MAPPING    0x063F
#define IQS_REG_TX_MAPPING    0x0649

#define IQS_REG_X_RESOLUTION  0x066E
#define IQS_REG_Y_RESOLUTION  0x0670

#define IQS_REG_PROX_THRESHOLD      0x0594
#define IQS_REG_SNAP_THRESHOLD      0x0592

#define IQS_REG_ACTIVE_CHANNELS     0x067B
#define IQS_REG_ATI_TARGET          0x056D
#define IQS_ATI_TARGET              800

#define IQS5XX_ACTIVE_CHANNELS_ADDR  0x067B
#define IQS5XX_ACTIVE_CHANNELS_SIZE  30      // 15 Tx rows × 2 bytes each

/** Data Sizes */
#define IQS_COORD_SIZE                  2     // Bytes per coordinate
#define IQS_DEVICE_INFO_SIZE            6
#define IQS_DELTA_START_ADDR   0x01C1
#define IQS_DELTA_LENGTH       300
#define IQS_TX_SIZE                     15
#define IQS_RX_SIZE                     10
#define IQS_I2C_RETRY_COUNT     3
#define IQS_I2C_RETRY_DELAY_US  1000  // 1ms delay between retries

//#define IQS_REG_COUNT_VALUES 300
#define IQS_REG_DELTA_VALUES 300

/*============================================================================
 * Custom PCB Configuration Constants
 *===========================================================================*/
#define IQS_TX_SIZE                     15      // Tx0 - Tx14
#define IQS_RX_SIZE                     10      // Rx0 - Rx9
#define IQS_TOTAL_CHANNELS              (IQS_TX_SIZE * IQS_RX_SIZE)  // 150 channels

// Resolution = (channels - 1) * 256
#define IQS_DEFAULT_X_RES               ((IQS_TX_SIZE - 1) * 256)    // 3584
#define IQS_DEFAULT_Y_RES               ((IQS_RX_SIZE - 1) * 256)    // 2304

// IQS SYS CONFIG BITS
#define IQS_SYS_CONFIG_0_REATI        (1U << 2)
#define IQS_SYS_CONFIG_0_ALP_REATI    (1U << 3)


// IQS SYS INFO Bits
#define IQS_SYS_INFO_0_ATI_ERROR      (1U << 3)
#define IQS_SYS_INFO_0_ALP_ATI_ERROR    (1U << 5)


/** IQS Device Info **/
#define IQS_PROJECT_NUM 15
#define IQS550_PRODUCT_NUM 40
#define IQS572_PRODUCT_NUM 58


/** IQS Bits **/
#define IQS_SYS_CONFIG_PROX_PIN           (1U << 7)
#define IQS_SYS_CONFIG_GES_PIN             (1U << 1)
#define IQS_SYS_CONFIG_TP_PIN              (1U << 2)
#define IQS_SYS_CONFIG_EVENT_MODE_PIN     (1U << 0)
#define IQS_SYS_CONTROL_1_RESET_PIN         (1U << 1)

#define IQS_SYS_CONFIG_0_SETUP_COMPLETE_PIN   (1U << 6)


//#define IQS_SYS_INFO0_ATI_ERROR             (1U << 3)

/** IQS System Control flags **/
#define IQS_SYS_CONTROL_0_AUTO_ATI          (1U << 5)

/** IQS Gesture Event 0 flags **/
#define IQS_SINGLE_TAP                      (1U << 0)
#define IQS_PRESS_AND_HOLD                  (1U << 1)
#define IQS_SWIPE_Xn                        (1U << 2)
#define IQS_SWIPE_Xp                        (1U << 3)
#define IQS_SWIPE_Yp                        (1U << 4)
#define IQS_SWIPE_Yn                        (1U << 5)

/** IQS Gesture Event 1 Flags **/
#define IQS_2_FINGER_TAP                     (1U << 0)
#define IQS_SCROLL                           (1U << 1)
#define IQS_ZOOM                             (1U << 2)
#define IQS_SINGLE_FINGER_GESTURE_EN_MASK     0x3f
#define IQS_MULTI_FINGER_GESTURE_EN_MASK      0x07

iqs_error_e iqs_read_register(uint16_t reg_addr, uint8_t *data, size_t len);
iqs_error_e iqs_write_register(uint16_t reg_addr, uint8_t *data, size_t len);
iqs_error_e iqs_check_setup_window(void);
const char* iqs_error_to_str(iqs_error_e err);
iqs_error_e iqs_init(void);
iqs_error_e iqs_channel_config(void);
iqs_error_e iqs_sys_config(void);
iqs_error_e iqs_auto_ATI(void);
iqs_error_e iqs_ack_reset(void);
iqs_error_e iqs_gesture_enable(void);
iqs_error_e iqs_reset(void);
iqs_error_e iqs_read_sys_flags(void);
iqs_error_e iqs_get_device_info(iqs_device_info_t *info);
iqs_error_e iqs_get_rel_coordinates(iqs_coordinates_t *coords);
iqs_error_e iqs_end_of_communication(void);
iqs_error_e iqs_read_all_events(iqs_full_events_t *events);
iqs_error_e iqs_read_delta_values(uint8_t *delta_buf);
iqs_error_e iqs_set_total_tx_rx(void);
iqs_error_e iqs_gesture_disable(void);
iqs_error_e iqs_configure_channels(const iqs_channel_config_t *config);
void iqs_get_default_channel_config(iqs_channel_config_t *config);
iqs_error_e iqs_check_threshold(void);
iqs_error_e iqs5xx_configure_active_channels(void);

//iqs_info_e iqs_check_version();
//
//void iqs_get_xy_cord(uint8_t *x_coord, uint8_t *y_coord);
//
//void iqs_init();
//
//void iqs_get_xy();


#endif /* INC_IQS572_H_ */
