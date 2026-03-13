/**
 * @file iqs550_driver.h
 * @brief IQS550 Touch Controller Driver for EFR32
 * @details Initialization and configuration for custom PCB with Tx(0-14), Rx(0-9)
 *
 * Based on IQS5xx-B000 datasheet from Azoteq
 */

#ifndef IQS550_DRIVER_H
#define IQS550_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "app.h"

/*============================================================================
 * IQS550 I2C Configuration
 *===========================================================================*/
#define IQS_I2C_ADDR                    0x74    // Default 7-bit I2C address

/*============================================================================
 * IQS550 Product Information
 *===========================================================================*/
#define IQS550_PROD_NUM                 40      // Product number for IQS550
#define IQS550_PROJ_NUM_B000            15      // B000 firmware project number
#define IQS550_MAJOR_VER_MIN            2       // Minimum major version

/*============================================================================
 * Custom PCB Configuration - Tx(0-14), Rx(0-9)
 *===========================================================================*/
#define IQS550_TOTAL_TX_CHANNELS        15      // Tx0 - Tx14
#define IQS550_TOTAL_RX_CHANNELS        10      // Rx0 - Rx9
#define IQS550_TOTAL_CHANNELS           (IQS550_TOTAL_TX_CHANNELS * IQS550_TOTAL_RX_CHANNELS)  // 150 channels

// Resolution calculation: (channels - 1) * 256
#define IQS550_X_RESOLUTION             ((IQS550_TOTAL_TX_CHANNELS - 1) * 256)  // 3584
#define IQS550_Y_RESOLUTION             ((IQS550_TOTAL_RX_CHANNELS - 1) * 256)  // 2304

/*============================================================================
 * IQS550 Register Map (16-bit addresses for B000 firmware)
 *===========================================================================*/

// Device Information Registers (Read-only)
#define IQS550_REG_PROD_NUM             0x0000  // Product number (2 bytes)
#define IQS550_REG_PROJ_NUM             0x0002  // Project number (2 bytes)
#define IQS550_REG_MAJOR_VER            0x0004  // Major version
#define IQS550_REG_MINOR_VER            0x0005  // Minor version
#define IQS550_REG_BL_STATUS            0x0006  // Bootloader status

// Gesture and Status Registers
#define IQS550_REG_SYS_INFO0            0x000F  // System info 0 (flags)
#define IQS550_REG_SYS_INFO1            0x0010  // System info 1 (touch count)
#define IQS550_REG_NUM_FINGERS          0x0011  // Number of fingers detected
#define IQS550_REG_REL_X                0x0012  // Relative X (2 bytes)
#define IQS550_REG_REL_Y                0x0014  // Relative Y (2 bytes)
#define IQS550_REG_ABS_X                0x0016  // Absolute X finger 1 (2 bytes)
#define IQS550_REG_ABS_Y                0x0018  // Absolute Y finger 1 (2 bytes)
#define IQS550_REG_TOUCH_STR            0x001A  // Touch strength finger 1 (2 bytes)
#define IQS550_REG_TOUCH_AREA           0x001C  // Touch area finger 1

// Multi-touch coordinates (5 fingers max)
#define IQS550_REG_FINGER1_X            0x0016
#define IQS550_REG_FINGER1_Y            0x0018
#define IQS550_REG_FINGER2_X            0x001F
#define IQS550_REG_FINGER2_Y            0x0021
#define IQS550_REG_FINGER3_X            0x0028
#define IQS550_REG_FINGER3_Y            0x002A
#define IQS550_REG_FINGER4_X            0x0031
#define IQS550_REG_FINGER4_Y            0x0033
#define IQS550_REG_FINGER5_X            0x003A
#define IQS550_REG_FINGER5_Y            0x003C

// Gesture Registers
#define IQS550_REG_GESTURE_EVENTS0      0x000D  // Single finger gestures
#define IQS550_REG_GESTURE_EVENTS1      0x000E  // Multi-finger gestures

// System Control Registers
#define IQS550_REG_SYS_CTRL0            0x0431  // System control 0
#define IQS550_REG_SYS_CTRL1            0x0432  // System control 1

// System Configuration Registers
#define IQS550_REG_SYS_CFG0             0x058E  // System config 0
#define IQS550_REG_SYS_CFG1             0x058F  // System config 1

// Active Channel Configuration
#define IQS550_REG_ACTIVE_CH            0x063D  // Active channels setup start
#define IQS550_REG_TOTAL_RX             0x063D  // Total Rx channels
#define IQS550_REG_TOTAL_TX             0x063E  // Total Tx channels

// Tx/Rx Enable Registers (16-bit bitmasks)
#define IQS550_REG_RX_ENABLE_L          0x063F  // Rx enable low byte (Rx0-7)
#define IQS550_REG_RX_ENABLE_H          0x0640  // Rx enable high byte (Rx8-9)
#define IQS550_REG_TX_ENABLE_L          0x0641  // Tx enable low byte (Tx0-7)
#define IQS550_REG_TX_ENABLE_H          0x0642  // Tx enable high byte (Tx8-14)

// Tx/Rx Mapping Registers
#define IQS550_REG_RX_MAPPING           0x0643  // Rx mapping start (10 bytes for IQS550)
#define IQS550_REG_TX_MAPPING           0x064D  // Tx mapping start (15 bytes for IQS550)

// Resolution Registers
#define IQS550_REG_X_RES                0x066E  // X resolution (2 bytes)
#define IQS550_REG_Y_RES                0x0670  // Y resolution (2 bytes)

// Orientation/Axis Configuration
#define IQS550_REG_XY_CONFIG            0x0669  // XY axis configuration

// ATI (Auto-Tuning Implementation) Registers
#define IQS550_REG_ATI_TARGET           0x0590  // ATI target (2 bytes)
#define IQS550_REG_ATI_C                0x0592  // ATI compensation
#define IQS550_REG_REF_DRIFT_LIMIT      0x0594  // Reference drift limit
#define IQS550_REG_LP_UPDATE_RATE       0x0596  // Low power update rate
#define IQS550_REG_NORMAL_UPDATE_RATE   0x0598  // Normal update rate

// Threshold Registers
#define IQS550_REG_PROX_THRESH          0x05A0  // Proximity threshold
#define IQS550_REG_TOUCH_THRESH         0x05A2  // Touch threshold multiplier
#define IQS550_REG_SNAP_THRESH          0x05A4  // Snap threshold

// Filter Registers
#define IQS550_REG_STATIC_FILTER        0x05A6  // Static filter beta
#define IQS550_REG_DYNAMIC_FILTER_UPPER 0x05A7  // Dynamic filter upper
#define IQS550_REG_DYNAMIC_FILTER_LOWER 0x05A8  // Dynamic filter lower

// Timing Configuration
#define IQS550_REG_TIMEOUT_IDLE         0x05B0  // Idle timeout
#define IQS550_REG_TIMEOUT_LP           0x05B2  // Low power timeout

// Channel Status/Data Registers (based on image in datasheet)
// For 15x10 configuration:
// Address X = Status/Config [Row0] High Byte
// Address X+1 = Status/Config [Row0] Low Byte
// ... continues for each row
#define IQS550_REG_CH_STATUS_BASE       0x0000  // Channel status base (varies)

// Count/Delta/Reference Data (2 bytes per channel)
#define IQS550_REG_COUNT_DATA           0x0100  // Count values start
#define IQS550_REG_DELTA_DATA           0x0200  // Delta values start
#define IQS550_REG_REF_DATA             0x0300  // Reference values start

// Export/Configuration File
#define IQS550_REG_EXP_FILE             0x0677  // Export file info

// Checksum and Application
#define IQS550_REG_CHKSM                0x83C0  // Checksum
#define IQS550_REG_APP                  0x8400  // Application start
#define IQS550_REG_CSTM                 0xBE00  // Custom settings

/*============================================================================
 * System Info 0 (SYS_INFO0) Bit Definitions
 *===========================================================================*/
#define IQS550_SYS_INFO0_SHOW_RESET     (1 << 7)  // Reset occurred flag
#define IQS550_SYS_INFO0_ALP_ATI_ERROR  (1 << 6)  // ALP ATI error
#define IQS550_SYS_INFO0_ATI_ERROR      (1 << 5)  // ATI error
#define IQS550_SYS_INFO0_ALP_PROX       (1 << 4)  // ALP proximity detected
#define IQS550_SYS_INFO0_GESTURE        (1 << 3)  // Gesture event
#define IQS550_SYS_INFO0_TP_MOVEMENT    (1 << 2)  // Trackpad movement
#define IQS550_SYS_INFO0_TOO_MANY_FNGRS (1 << 1)  // Too many fingers
#define IQS550_SYS_INFO0_PALM_DETECT    (1 << 0)  // Palm detected

/*============================================================================
 * System Control 0 (SYS_CTRL0) Bit Definitions
 *===========================================================================*/
#define IQS550_SYS_CTRL0_ACK_RESET      (1 << 7)  // Acknowledge reset
#define IQS550_SYS_CTRL0_SW_RESET       (1 << 1)  // Software reset
#define IQS550_SYS_CTRL0_SUSPEND        (1 << 0)  // Suspend mode

/*============================================================================
 * System Control 1 (SYS_CTRL1) Bit Definitions
 *===========================================================================*/
#define IQS550_SYS_CTRL1_TP_RESEED      (1 << 7)  // Trackpad reseed
#define IQS550_SYS_CTRL1_ALP_RESEED     (1 << 6)  // ALP reseed
#define IQS550_SYS_CTRL1_REATI          (1 << 2)  // Re-ATI trigger

/*============================================================================
 * System Config 0 (SYS_CFG0) Bit Definitions
 *===========================================================================*/
#define IQS550_SYS_CFG0_SETUP_COMPLETE  (1 << 6)  // Setup complete flag
#define IQS550_SYS_CFG0_WDT_ENABLE      (1 << 5)  // Watchdog enable
#define IQS550_SYS_CFG0_ALP_REATI       (1 << 3)  // ALP Re-ATI enable
#define IQS550_SYS_CFG0_REATI           (1 << 2)  // Re-ATI enable
#define IQS550_SYS_CFG0_IO_WAKEUP       (1 << 1)  // I/O wakeup enable
#define IQS550_SYS_CFG0_EVENT_MODE      (1 << 0)  // Event mode enable

/*============================================================================
 * System Config 1 (SYS_CFG1) Bit Definitions
 *===========================================================================*/
#define IQS550_SYS_CFG1_TP_EVENT        (1 << 2)  // Trackpad event mode
#define IQS550_SYS_CFG1_PROX_EVENT      (1 << 1)  // Proximity event mode
#define IQS550_SYS_CFG1_TOUCH_EVENT     (1 << 0)  // Touch event mode

/*============================================================================
 * XY Config Bit Definitions
 *===========================================================================*/
#define IQS550_XY_CFG_FLIP_X            (1 << 0)  // Flip X axis
#define IQS550_XY_CFG_FLIP_Y            (1 << 1)  // Flip Y axis
#define IQS550_XY_CFG_SWITCH_XY         (1 << 2)  // Switch X/Y axes

/*============================================================================
 * Gesture Event Definitions (Single Finger)
 *===========================================================================*/
#define IQS550_GESTURE_SINGLE_TAP       (1 << 0)
#define IQS550_GESTURE_TAP_AND_HOLD     (1 << 1)
#define IQS550_GESTURE_SWIPE_X_NEG      (1 << 2)
#define IQS550_GESTURE_SWIPE_X_POS      (1 << 3)
#define IQS550_GESTURE_SWIPE_Y_NEG      (1 << 4)
#define IQS550_GESTURE_SWIPE_Y_POS      (1 << 5)

/*============================================================================
 * Gesture Event Definitions (Multi Finger)
 *===========================================================================*/
#define IQS550_GESTURE_TWO_FINGER_TAP   (1 << 0)
#define IQS550_GESTURE_SCROLL           (1 << 1)
#define IQS550_GESTURE_ZOOM             (1 << 2)

/*============================================================================
 * Error Codes
 *===========================================================================*/
typedef enum {
    IQS_OK = 0,
    IQS_ERR_INVALID_PARAM,
    IQS_ERR_I2C_READ_FAIL,
    IQS_ERR_I2C_WRITE_FAIL,
    IQS_ERR_TIMEOUT,
    IQS_ERR_INVALID_DEVICE,
    IQS_ERR_ATI_FAILED,
    IQS_ERR_NOT_READY,
    IQS_ERR_WRONG_FIRMWARE,
    IQS_ERR_INIT_FAILED,
} iqs_error_e;

/*============================================================================
 * Data Structures
 *===========================================================================*/

// Device identification info
typedef struct {
    uint16_t prod_num;
    uint16_t proj_num;
    uint8_t  major_ver;
    uint8_t  minor_ver;
    uint8_t  bl_status;
} iqs550_dev_id_t;

// Single touch point data
typedef struct {
    uint16_t abs_x;
    uint16_t abs_y;
    uint16_t touch_strength;
    uint8_t  touch_area;
} iqs550_touch_point_t;

// Touch data structure (up to 5 fingers)
typedef struct {
    uint8_t num_fingers;
    int16_t rel_x;
    int16_t rel_y;
    iqs550_touch_point_t points[5];
} iqs550_touch_data_t;

// System status
typedef struct {
    bool reset_occurred;
    bool ati_error;
    bool alp_ati_error;
    bool gesture_event;
    bool tp_movement;
    bool palm_detected;
    uint8_t gesture_single;
    uint8_t gesture_multi;
} iqs550_status_t;

// Channel configuration for custom PCB
typedef struct {
    uint8_t  total_rx;           // Number of Rx channels (0-9 = 10)
    uint8_t  total_tx;           // Number of Tx channels (0-14 = 15)
    uint16_t rx_enable;          // Bitmask for enabled Rx (bits 0-9)
    uint16_t tx_enable;          // Bitmask for enabled Tx (bits 0-14)
    uint8_t  rx_mapping[10];     // Rx mapping table
    uint8_t  tx_mapping[15];     // Tx mapping table
    uint16_t x_resolution;       // X resolution
    uint16_t y_resolution;       // Y resolution
    uint8_t  xy_config;          // Axis flip/swap configuration
} iqs550_channel_config_t;

// ATI configuration
typedef struct {
    uint16_t ati_target;         // ATI target value
    uint8_t  ati_c;              // ATI compensation
    uint8_t  prox_threshold;     // Proximity threshold
    uint8_t  touch_threshold;    // Touch threshold multiplier
    uint8_t  snap_threshold;     // Snap threshold
} iqs550_ati_config_t;

// Complete IQS550 configuration
typedef struct {
    iqs550_channel_config_t channels;
    iqs550_ati_config_t     ati;
    bool                    event_mode;
    bool                    wdt_enable;
} iqs550_config_t;

/*============================================================================
 * Function Prototypes
 *===========================================================================*/

/**
 * @brief Initialize the IQS550 touch controller
 * @param config Pointer to configuration structure (NULL for defaults)
 * @return IQS_OK on success, error code otherwise
 */
iqs_error_e iqs550_init(const iqs550_config_t *config);

/**
 * @brief Get default configuration for Tx(0-14), Rx(0-9) PCB
 * @param config Pointer to configuration structure to fill
 */
void iqs550_get_default_config(iqs550_config_t *config);

/**
 * @brief Read device identification
 * @param dev_id Pointer to device ID structure
 * @return IQS_OK on success, error code otherwise
 */
iqs_error_e iqs550_read_device_id(iqs550_dev_id_t *dev_id);

/**
 * @brief Configure Tx/Rx channels
 * @param ch_config Pointer to channel configuration
 * @return IQS_OK on success, error code otherwise
 */
iqs_error_e iqs550_configure_channels(const iqs550_channel_config_t *ch_config);

/**
 * @brief Enable/disable specific Tx channel
 * @param tx_num Tx channel number (0-14)
 * @param enable true to enable, false to disable
 * @return IQS_OK on success, error code otherwise
 */
iqs_error_e iqs550_set_tx_enable(uint8_t tx_num, bool enable);

/**
 * @brief Enable/disable specific Rx channel
 * @param rx_num Rx channel number (0-9)
 * @param enable true to enable, false to disable
 * @return IQS_OK on success, error code otherwise
 */
iqs_error_e iqs550_set_rx_enable(uint8_t rx_num, bool enable);

/**
 * @brief Trigger ATI (Auto-Tuning Implementation)
 * @return IQS_OK on success, error code otherwise
 */
iqs_error_e iqs550_trigger_ati(void);

/**
 * @brief Wait for ATI to complete
 * @param timeout_ms Timeout in milliseconds
 * @return IQS_OK on success, IQS_ERR_TIMEOUT on timeout
 */
iqs_error_e iqs550_wait_ati_complete(uint32_t timeout_ms);

/**
 * @brief Read touch data
 * @param touch_data Pointer to touch data structure
 * @return IQS_OK on success, error code otherwise
 */
iqs_error_e iqs550_read_touch_data(iqs550_touch_data_t *touch_data);

/**
 * @brief Read system status
 * @param status Pointer to status structure
 * @return IQS_OK on success, error code otherwise
 */
iqs_error_e iqs550_read_status(iqs550_status_t *status);

/**
 * @brief Acknowledge reset flag
 * @return IQS_OK on success, error code otherwise
 */
iqs_error_e iqs550_ack_reset(void);

/**
 * @brief Software reset
 * @return IQS_OK on success, error code otherwise
 */
iqs_error_e iqs550_sw_reset(void);

/**
 * @brief Enter suspend mode
 * @return IQS_OK on success, error code otherwise
 */
iqs_error_e iqs550_suspend(void);

/**
 * @brief Resume from suspend
 * @return IQS_OK on success, error code otherwise
 */
iqs_error_e iqs550_resume(void);

/**
 * @brief Check if device is ready (RDY pin or register polling)
 * @return true if ready, false otherwise
 */
bool iqs550_is_ready(void);

/**
 * @brief Wait for device to be ready
 * @param timeout_ms Timeout in milliseconds
 * @return IQS_OK on success, IQS_ERR_TIMEOUT on timeout
 */
iqs_error_e iqs550_wait_ready(uint32_t timeout_ms);

/**
 * @brief Read raw channel count data
 * @param channel Channel number (0-149)
 * @param count Pointer to store count value
 * @return IQS_OK on success, error code otherwise
 */
iqs_error_e iqs550_read_channel_count(uint8_t channel, uint16_t *count);

/**
 * @brief Read raw channel delta data
 * @param channel Channel number (0-149)
 * @param delta Pointer to store delta value
 * @return IQS_OK on success, error code otherwise
 */
iqs_error_e iqs550_read_channel_delta(uint8_t channel, int16_t *delta);

#endif // IQS550_DRIVER_H
