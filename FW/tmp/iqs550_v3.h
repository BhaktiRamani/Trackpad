/*
 * iqs550.h
 *
 * IQS550-B000 Touch Controller Driver for Custom PCB
 * Configuration: Tx(0-14) x Rx(0-9) = 150 channels
 *
 * Based on original iqs572.h by Bhakti Ramani
 * Modified for IQS550 custom PCB
 */

#ifndef INC_IQS550_H_
#define INC_IQS550_H_

#include "app.h"

/*============================================================================
 * Device Information Enums
 *===========================================================================*/
typedef enum iqs_info {
    fail = -1,
    project_num = 15,           // B000 firmware
    product_num_550 = 40,       // IQS550
    product_num_572 = 58,       // IQS572
} iqs_info_e;

/*============================================================================
 * Data Structures
 *===========================================================================*/
typedef struct {
    uint16_t product_num;
    uint16_t project_num;
    uint8_t major_ver;
    uint8_t minor_ver;
} iqs_device_info_t;

typedef struct {
    int16_t x;
    int16_t y;
} iqs_coordinates_t;

typedef struct {
    uint16_t abs_x;
    uint16_t abs_y;
    uint16_t touch_strength;
    uint8_t touch_area;
} iqs_touch_point_t;

typedef struct {
    uint8_t gesture_events_0;
    uint8_t gesture_events_1;
    uint8_t system_info_0;
    uint8_t system_info_1;
    uint8_t num_fingers;
    int16_t rel_x;
    int16_t rel_y;
    iqs_touch_point_t finger[5];    // Up to 5 fingers
    bool single_tap;
    bool press_and_hold;
    bool double_tap;
    bool scroll;
    bool zoom;
} iqs_full_events_t;

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

/*============================================================================
 * I2C Device Address
 *===========================================================================*/
#define IQS_I2C_ADDR                    0x74    // 0xE8 >> 1

/*============================================================================
 * Register Addresses - From IQS5xx-B000 Memory Map
 * (Corrected based on datasheet screenshots)
 *===========================================================================*/

/* Device Information Registers (0x0000 - 0x0006) */
#define IQS_REG_DEVICE_INFO             0x0000  // Start of device info block
#define IQS_REG_PRODUCT_NUM             0x0000  // Product number (2 bytes)
#define IQS_REG_PROJECT_NUM             0x0002  // Project number (2 bytes)
#define IQS_REG_MAJOR_VER               0x0004  // Major version (1 byte)
#define IQS_REG_MINOR_VER               0x0005  // Minor version (1 byte)
#define IQS_REG_BOOTLOADER_STATUS       0x0006  // Bootloader status (1 byte)

/* Trackpad Info (0x000B - 0x000C) */
#define IQS_REG_MAX_TOUCH_COL           0x000B  // Max touch column (high byte)
#define IQS_REG_MAX_TOUCH_ROW           0x000B  // Max touch row (low byte) - same register
#define IQS_REG_PREV_CYCLE_TIME         0x000C  // Previous cycle time [ms]

/* Gesture & System Events (0x000D - 0x0010) */
#define IQS_REG_GESTURE_EVENTS_0        0x000D  // Single finger gestures
#define IQS_REG_GESTURE_EVENTS_1        0x000E  // Multi-finger gestures
#define IQS_REG_SYSTEM_INFO_0           0x000F  // System info 0 (flags)
#define IQS_REG_SYSTEM_INFO_1           0x0010  // System info 1

/* Touch Data (0x0011 - 0x001C for finger 1) */
#define IQS_REG_NUM_FINGERS             0x0011  // Number of fingers detected
#define IQS_REG_REL_X                   0x0012  // Relative X (2 bytes)
#define IQS_REG_REL_Y                   0x0014  // Relative Y (2 bytes)
#define IQS_REG_ABS_X                   0x0016  // Absolute X finger 1 (2 bytes)
#define IQS_REG_ABS_Y                   0x0018  // Absolute Y finger 1 (2 bytes)
#define IQS_REG_TOUCH_STRENGTH          0x001A  // Touch strength finger 1 (2 bytes)
#define IQS_REG_TOUCH_AREA              0x001C  // Touch area/size finger 1

/* Multi-finger data (fingers 2-5 at 0x0038+) */
#define IQS_REG_FINGERS_2_5_DATA        0x0038  // Touch strength/area for fingers 2-5

/* Channel Status Data */
#define IQS_REG_PROX_STATUS             0x0039  // Prox status (32 bytes)
#define IQS_REG_TOUCH_STATUS            0x0059  // Touch status (30 bytes)
#define IQS_REG_SNAP_STATUS             0x0077  // Snap status (30 bytes)
#define IQS_REG_COUNT_VALUES            0x0095  // Count values (300 bytes)
#define IQS_REG_DELTA_VALUES            0x01C1  // Delta values (300 bytes)
#define IQS_REG_ALP_COUNT               0x02ED  // ALP count value (2 bytes)
#define IQS_REG_ALP_IND_COUNT           0x02EF  // ALP individual count values (20 bytes)
#define IQS_REG_REFERENCE_VALUES        0x0303  // Reference values (300 bytes)

/* ALP (Alternate Low Power) Registers */
#define IQS_REG_ALP_LTA                 0x042F  // ALP LTA (2 bytes)

/* System Control Registers (0x0431 - 0x0432) */
#define IQS_REG_SYS_CONTROL_0           0x0431  // System Control 0
#define IQS_REG_SYS_CONTROL_1           0x0432  // System Control 1

/* ATI Compensation (0x0435 - 0x056A) */
#define IQS_REG_ALP_ATI_COMP            0x0435  // ALP ATI compensation (10 bytes)
#define IQS_REG_ATI_COMP                0x043F  // ATI compensation (150 bytes)
#define IQS_REG_ATI_C_IND_ADJ           0x04D5  // ATI C individual adjust (150 bytes)

/* Global ATI Settings (0x056B - 0x0572) */
#define IQS_REG_GLOBAL_ATI_C            0x056B  // Global ATI C
#define IQS_REG_ALP_ATI_C               0x056C  // ALP ATI C
#define IQS_REG_ATI_TARGET              0x056D  // ATI target (2 bytes)
#define IQS_REG_ALP_ATI_TARGET          0x056F  // ALP ATI target (2 bytes)
#define IQS_REG_REF_DRIFT_LIMIT         0x0571  // Reference drift limit
#define IQS_REG_ALP_LTA_DRIFT_LIMIT     0x0572  // ALP LTA drift limit

/* Timing & Compensation Limits (0x0573 - 0x0589) */
#define IQS_REG_REATI_LOWER_COMP        0x0573  // Re-ATI lower compensation limit
#define IQS_REG_REATI_UPPER_COMP        0x0574  // Re-ATI upper compensation limit
#define IQS_REG_MAX_COUNT_LIMIT         0x0575  // Max count limit (2 bytes)
#define IQS_REG_REATI_RETRY_TIME        0x0577  // Re-ATI retry time [s]
#define IQS_REG_REPORT_RATE_ACTIVE      0x057A  // Report rate [ms] - Active mode (2 bytes)
#define IQS_REG_REPORT_RATE_IDLE_TOUCH  0x057C  // Report rate [ms] - Idle touch mode (2 bytes)
#define IQS_REG_REPORT_RATE_IDLE        0x057E  // Report rate [ms] - Idle mode (2 bytes)
#define IQS_REG_REPORT_RATE_LP1         0x0580  // Report rate [ms] - LP1 mode (2 bytes)
#define IQS_REG_REPORT_RATE_LP2         0x0582  // Report rate [ms] - LP2 mode (2 bytes)
#define IQS_REG_TIMEOUT_ACTIVE          0x0584  // Timeout [s] - Active
#define IQS_REG_TIMEOUT_IDLE_TOUCH      0x0585  // Timeout [s] - Idle touch
#define IQS_REG_TIMEOUT_IDLE            0x0586  // Timeout [s] - Idle mode
#define IQS_REG_TIMEOUT_LP1             0x0587  // Timeout [x 20s] - LP1 mode
#define IQS_REG_REF_UPDATE_TIME         0x0588  // Reference update time [s]
#define IQS_REG_SNAP_TIMEOUT            0x0589  // Snap timeout [s]
#define IQS_REG_I2C_TIMEOUT             0x058A  // I2C timeout [ms]

/* System Configuration Registers (0x058E - 0x058F) */
#define IQS_REG_SYS_CONFIG_0            0x058E  // System Config 0
#define IQS_REG_SYS_CONFIG_1            0x058F  // System Config 1

/* Threshold Registers (0x0590 - 0x0597) */
#define IQS_REG_SNAP_THRESHOLD          0x0592  // Snap threshold (2 bytes)
#define IQS_REG_PROX_THRESHOLD_TP       0x0594  // Prox threshold - trackpad
#define IQS_REG_PROX_THRESHOLD_ALP      0x0595  // Prox threshold - ALP channel
#define IQS_REG_TOUCH_MULT_SET          0x0596  // Global touch multiplier - set
#define IQS_REG_TOUCH_MULT_CLEAR        0x0597  // Global touch multiplier - clear

/* Individual Touch Multiplier (0x0598 - 0x062D) */
#define IQS_REG_IND_TOUCH_MULT_ADJ      0x0598  // Individual touch multiplier adjustments (150 bytes)

/* Filter Settings (0x0632 - 0x063C) */
#define IQS_REG_FILTER_SETTINGS_0       0x0632  // Filter Settings 0
#define IQS_REG_XY_STATIC_BETA          0x0633  // XY static beta
#define IQS_REG_ALP_COUNT_BETA          0x0634  // ALP count beta
#define IQS_REG_ALP1_LTA_BETA           0x0635  // ALP1 LTA beta
#define IQS_REG_ALP2_LTA_BETA           0x0636  // ALP2 LTA beta
#define IQS_REG_XY_DYN_FILTER_BOT       0x0637  // XY dynamic filter - bottom beta
#define IQS_REG_XY_DYN_FILTER_LOW       0x0638  // XY dynamic filter - lower speed
#define IQS_REG_XY_DYN_FILTER_UP        0x0639  // XY dynamic filter - upper speed (2 bytes)

/*============================================================================
 * Channel Setup Registers (0x063D - 0x0657) - CRITICAL FOR CUSTOM PCB
 *===========================================================================*/
#define IQS_REG_TOTAL_RX                0x063D  // Total Rx channels
#define IQS_REG_TOTAL_TX                0x063E  // Total Tx channels
#define IQS_REG_RX_MAPPING              0x063F  // Rx mapping (10 bytes for IQS550)
#define IQS_REG_TX_MAPPING              0x0649  // Tx mapping (15 bytes for IQS550)

/* ALP Channel Setup (0x0658 - 0x065D) */
#define IQS_REG_ALP_CH_SETUP_0          0x0658  // ALP Channel Setup 0
#define IQS_REG_ALP_RX_SELECT           0x0659  // ALP Rx Select
#define IQS_REG_ALP_TX_SELECT           0x065B  // ALP Tx Select
#define IQS_REG_RX_TO_TX                0x065D  // Rx to Tx mapping

/* Hardware Settings (0x065E - 0x0667) */
#define IQS_REG_HARDWARE_SETTINGS_A     0x065E  // Hardware Settings A
#define IQS_REG_HARDWARE_SETTINGS_B     0x0660  // Hardware Settings B

/* XY Configuration (0x0668 - 0x066F) */
#define IQS_REG_XY_CONFIG               0x0668  // XY Config (PALM_REJECT, SWITCH_XY, FLIP_Y, FLIP_X)
#define IQS_REG_MAX_MULTI_TOUCHES       0x0669  // Max multi-touches
#define IQS_REG_FINGER_SPLIT_FACTOR     0x066A  // Finger split aggression factor
#define IQS_REG_FINGER_MERGE_LIMIT      0x066B  // Finger merge maximum limit
#define IQS_REG_X_RESOLUTION            0x066C  // X Resolution (2 bytes)
#define IQS_REG_Y_RESOLUTION            0x066E  // Y Resolution (2 bytes)

/* Touch Behavior (0x0670 - 0x0678) */
#define IQS_REG_STAT_TOUCH_THRESH       0x0672  // Stationary touch movement threshold (pixels)
#define IQS_REG_DEFAULT_READ_ADDR       0x0675  // Default read address (2 bytes)
#define IQS_REG_EXPORT_VER_NUM          0x0677  // Export version number (2 bytes)

/* Prox/Snap Disable & Active Channels (0x0679 - 0x067B) */
#define IQS_REG_PROX_DB_SET             0x0679  // Prox disable - Set
#define IQS_REG_PROX_DB_CLEAR           0x0679  // Prox disable - Clear (same byte, different bits)
#define IQS_REG_TOUCH_DB_CLEAR          0x067A  // Touch disable - Clear
#define IQS_REG_SNAP_EN_CHANNELS        0x067A  // Snap enabled channels (30 bytes)

/* Gesture Configuration Registers (0x06B7 - 0x06CF+) */
#define IQS_REG_SINGLE_FINGER_GESTURE_EN    0x06B7  // Single finger gesture enable
#define IQS_REG_MULTI_FINGER_GESTURE_EN     0x06B8  // Multi-finger gesture enable

/* Gesture Timing Parameters */
#define IQS_REG_TAP_TIME                0x06B9  // Tap time [ms] (2 bytes)
#define IQS_REG_TAP_DISTANCE            0x06BB  // Tap distance [pixels] (2 bytes)
#define IQS_REG_HOLD_TIME               0x06BD  // Hold time [ms] (2 bytes)
#define IQS_REG_SWIPE_INIT_TIME         0x06BF  // Swipe initial time [ms] (2 bytes)
#define IQS_REG_SWIPE_INIT_DISTANCE     0x06C1  // Swipe initial distance [pixels] (2 bytes)
#define IQS_REG_SWIPE_CONSEC_TIME       0x06C3  // Swipe consecutive time [ms] (2 bytes)
#define IQS_REG_SWIPE_CONSEC_DISTANCE   0x06C5  // Swipe consecutive distance [pixels] (2 bytes)
#define IQS_REG_SWIPE_ANGLE             0x06C7  // Swipe angle [degrees]
#define IQS_REG_SCROLL_INIT_DISTANCE    0x06C9  // Scroll initial distance [pixels] (2 bytes)
#define IQS_REG_SCROLL_ANGLE            0x06CB  // Scroll angle [tolerance]
#define IQS_REG_ZOOM_INIT_DISTANCE      0x06CD  // Zoom initial distance [pixels] (2 bytes)
#define IQS_REG_ZOOM_CONSEC_DISTANCE    0x06CF  // Zoom consecutive distance [pixels] (2 bytes)

/* End of Communication */
#define IQS_END_OF_COMMUNICATION        0xEEEE

/*============================================================================
 * Custom PCB Configuration Constants
 *===========================================================================*/
#define IQS_TX_SIZE                     15      // Tx0 - Tx14
#define IQS_RX_SIZE                     10      // Rx0 - Rx9
#define IQS_TOTAL_CHANNELS              (IQS_TX_SIZE * IQS_RX_SIZE)  // 150 channels

// Resolution = (channels - 1) * 256
#define IQS_DEFAULT_X_RES               ((IQS_TX_SIZE - 1) * 256)    // 3584
#define IQS_DEFAULT_Y_RES               ((IQS_RX_SIZE - 1) * 256)    // 2304

/*============================================================================
 * Data Sizes
 *===========================================================================*/
#define IQS_COORD_SIZE                  2       // Bytes per coordinate
#define IQS_DEVICE_INFO_SIZE            6       // Product(2) + Project(2) + Major(1) + Minor(1)
#define IQS_DELTA_START_ADDR            0x01C1
#define IQS_DELTA_LENGTH                300     // 150 channels * 2 bytes
#define IQS_COUNT_LENGTH                300
#define IQS_REF_LENGTH                  300

/*============================================================================
 * Device Identification Values
 *===========================================================================*/
#define IQS_PROJECT_NUM                 15      // B000 firmware
#define IQS550_PRODUCT_NUM              40
#define IQS572_PRODUCT_NUM              58

/*============================================================================
 * System Info 0 Bit Definitions (0x000F)
 *===========================================================================*/
#define IQS_SYS_INFO0_SHOW_RESET        (1U << 7)   // Reset occurred
#define IQS_SYS_INFO0_ALP_ATI_ERROR     (1U << 6)   // ALP ATI error
#define IQS_SYS_INFO0_ATI_ERROR         (1U << 5)   // ATI error
#define IQS_SYS_INFO0_ALP_REATI_OCCURRED (1U << 4)  // ALP Re-ATI occurred
#define IQS_SYS_INFO0_REATI_OCCURRED    (1U << 3)   // Re-ATI occurred
#define IQS_SYS_INFO0_CHARGING_MODE     (1U << 2)   // Charging mode active
// Bits 0-1 reserved

/*============================================================================
 * System Info 1 Bit Definitions (0x0010)
 *===========================================================================*/
#define IQS_SYS_INFO1_SWITCH_STATE      (1U << 7)   // Switch state
#define IQS_SYS_INFO1_SNAP_TOGGLE       (1U << 6)   // Snap toggle
#define IQS_SYS_INFO1_RR_MISSED         (1U << 5)   // Report rate missed
#define IQS_SYS_INFO1_TOO_MANY_FINGERS  (1U << 4)   // Too many fingers
#define IQS_SYS_INFO1_PALM_DETECT       (1U << 3)   // Palm detected
#define IQS_SYS_INFO1_TP_MOVEMENT       (1U << 2)   // Trackpad movement
// Bits 0-1 reserved

/*============================================================================
 * System Control 0 Bit Definitions (0x0431)
 *===========================================================================*/
#define IQS_SYS_CTRL0_ACK_RESET         (1U << 7)   // Acknowledge reset
#define IQS_SYS_CTRL0_AUTO_ATI          (1U << 6)   // Trigger Auto ATI
#define IQS_SYS_CTRL0_ALP_RESEED        (1U << 5)   // ALP reseed
#define IQS_SYS_CTRL0_RESEED            (1U << 4)   // Reseed
// Bits 2-3: MODE_SELECT
#define IQS_SYS_CTRL0_MODE_SELECT_MASK  (0x03 << 2)
// Bits 0-1 reserved

/*============================================================================
 * System Control 1 Bit Definitions (0x0432)
 *===========================================================================*/
#define IQS_SYS_CTRL1_RESET             (1U << 1)   // Software reset
#define IQS_SYS_CTRL1_SUSPEND           (1U << 0)   // Suspend mode

/*============================================================================
 * System Config 0 Bit Definitions (0x058E)
 *===========================================================================*/
#define IQS_SYS_CFG0_MANUAL_CONTROL     (1U << 7)   // Manual control
#define IQS_SYS_CFG0_SETUP_COMPLETE     (1U << 6)   // Setup complete
#define IQS_SYS_CFG0_WDT                (1U << 5)   // Watchdog enable
#define IQS_SYS_CFG0_SW_INPUT_EVENT     (1U << 4)   // SW input event
#define IQS_SYS_CFG0_ALP_REATI          (1U << 3)   // ALP Re-ATI enable
#define IQS_SYS_CFG0_REATI              (1U << 2)   // Re-ATI enable
#define IQS_SYS_CFG0_SW_INPUT_SELECT    (1U << 1)   // SW input select
#define IQS_SYS_CFG0_SW_INPUT           (1U << 0)   // SW input

/*============================================================================
 * System Config 1 Bit Definitions (0x058F)
 *===========================================================================*/
#define IQS_SYS_CFG1_PROX_EVENT         (1U << 7)   // Proximity event enable
#define IQS_SYS_CFG1_TOUCH_EVENT        (1U << 6)   // Touch event enable
#define IQS_SYS_CFG1_SNAP_EVENT         (1U << 5)   // Snap event enable
#define IQS_SYS_CFG1_ALP_PROX_EVENT     (1U << 4)   // ALP proximity event enable
#define IQS_SYS_CFG1_REATI_EVENT        (1U << 3)   // Re-ATI event enable
#define IQS_SYS_CFG1_TP_EVENT           (1U << 2)   // Trackpad event enable
#define IQS_SYS_CFG1_GESTURE_EVENT      (1U << 1)   // Gesture event enable
#define IQS_SYS_CFG1_EVENT_MODE         (1U << 0)   // Event mode enable

/*============================================================================
 * XY Config Bit Definitions (0x0668)
 *===========================================================================*/
#define IQS_XY_CFG_PALM_REJECT          (1U << 4)   // Palm rejection enable
#define IQS_XY_CFG_SWITCH_XY_AXIS       (1U << 2)   // Switch X/Y axes
#define IQS_XY_CFG_FLIP_Y               (1U << 1)   // Flip Y axis
#define IQS_XY_CFG_FLIP_X               (1U << 0)   // Flip X axis

/*============================================================================
 * Gesture Event 0 Flags (Single Finger - 0x000D)
 *===========================================================================*/
#define IQS_GESTURE_SINGLE_TAP          (1U << 0)
#define IQS_GESTURE_PRESS_AND_HOLD      (1U << 1)
#define IQS_GESTURE_SWIPE_X_NEG         (1U << 2)
#define IQS_GESTURE_SWIPE_X_POS         (1U << 3)
#define IQS_GESTURE_SWIPE_Y_POS         (1U << 4)
#define IQS_GESTURE_SWIPE_Y_NEG         (1U << 5)

// Legacy names for compatibility
#define IQS_SINGLE_TAP                  IQS_GESTURE_SINGLE_TAP
#define IQS_PRESS_AND_HOLD              IQS_GESTURE_PRESS_AND_HOLD
#define IQS_SWIPE_Xn                    IQS_GESTURE_SWIPE_X_NEG
#define IQS_SWIPE_Xp                    IQS_GESTURE_SWIPE_X_POS
#define IQS_SWIPE_Yp                    IQS_GESTURE_SWIPE_Y_POS
#define IQS_SWIPE_Yn                    IQS_GESTURE_SWIPE_Y_NEG

/*============================================================================
 * Gesture Event 1 Flags (Multi-Finger - 0x000E)
 *===========================================================================*/
#define IQS_GESTURE_2_FINGER_TAP        (1U << 0)
#define IQS_GESTURE_SCROLL              (1U << 1)
#define IQS_GESTURE_ZOOM                (1U << 2)

// Legacy names for compatibility
#define IQS_2_FINGER_TAP                IQS_GESTURE_2_FINGER_TAP
#define IQS_SCROLL                      IQS_GESTURE_SCROLL
#define IQS_ZOOM                        IQS_GESTURE_ZOOM

/*============================================================================
 * Gesture Enable Masks
 *===========================================================================*/
#define IQS_SINGLE_FINGER_GESTURE_EN_MASK   0x3F    // All single finger gestures
#define IQS_MULTI_FINGER_GESTURE_EN_MASK    0x07    // All multi-finger gestures

/*============================================================================
 * Function Prototypes
 *===========================================================================*/

/* Utility */
const char* iqs_error_to_str(iqs_error_e err);

/* Initialization */
iqs_error_e iqs_init(void);
iqs_error_e iqs_init_custom_pcb(const iqs_channel_config_t *config);

/* Device Info */
iqs_error_e iqs_get_device_info(iqs_device_info_t *info);

/* System Control */
iqs_error_e iqs_ack_reset(void);
iqs_error_e iqs_reset(void);
iqs_error_e iqs_suspend(void);
iqs_error_e iqs_resume(void);

/* Configuration */
iqs_error_e iqs_sys_config(void);
iqs_error_e iqs_auto_ATI(void);
iqs_error_e iqs_gesture_enable(void);
iqs_error_e iqs_gesture_disable(void);

/* Channel Configuration - NEW for custom PCB */
iqs_error_e iqs_set_total_tx_rx(void);
iqs_error_e iqs_configure_channels(const iqs_channel_config_t *config);
iqs_error_e iqs_set_tx_mapping(const uint8_t *mapping, uint8_t len);
iqs_error_e iqs_set_rx_mapping(const uint8_t *mapping, uint8_t len);
iqs_error_e iqs_set_resolution(uint16_t x_res, uint16_t y_res);
iqs_error_e iqs_set_xy_config(uint8_t config);
void iqs_get_default_channel_config(iqs_channel_config_t *config);

/* Data Reading */
iqs_error_e iqs_read_sys_flags(void);
iqs_error_e iqs_read_all_events(iqs_full_events_t *events);
iqs_error_e iqs_get_rel_coordinates(iqs_coordinates_t *coords);
iqs_error_e iqs_get_abs_coordinates(iqs_coordinates_t *coords);
iqs_error_e iqs_read_delta_values(uint8_t *delta_buf);
iqs_error_e iqs_read_count_values(uint8_t *count_buf);

/* Communication */
iqs_error_e iqs_end_of_communication(void);

#endif /* INC_IQS550_H_ */