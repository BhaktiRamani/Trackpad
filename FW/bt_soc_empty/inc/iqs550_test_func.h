/*
 * iqs550_test_func.h
 *
 *  Created on: Nov 28, 2025
 *      Author: Bhakti Ramani
 */

#ifndef INC_IQS550_TEST_FUNC_H_
#define INC_IQS550_TEST_FUNC_H_

#include "app.h"

iqs_error_e iqs_auto_ATI_with_retry(void);
iqs_error_e iqs_set_ati_target(uint16_t target);
iqs_error_e iqs_read_system_info_0(uint8_t *sys_info);
iqs_error_e iqs_set_ati_target(uint16_t target);
iqs_error_e iqs_set_alp_ati_target(uint16_t target);
iqs_error_e iqs_set_reati_limits(uint8_t lower, uint8_t upper);
iqs_error_e iqs_read_ati_config(void);
iqs_error_e iqs_auto_ATI2(void);
iqs_error_e iqs_auto_ATI3(void);
int iqs_auto_ATI4();
int iqs_sys_config2(void);
iqs_sys_config_fixed();
iqs_configure_active_channels();
int iqs_auto_ATI_force_high_ati_c(void);
int iqs_read_ati_parameters(void);
int iqs_auto_ATI_with_device_defaults(void);

iqs_error_e iqs_read_all_count_values(void);
iqs_error_e iqs_read_all_delta_values(void);
iqs_error_e iqs_read_all_channel_data(void);
iqs_error_e iqs_read_channels_table(void);
iqs_error_e iqs_read_active_channels(void);

iqs_error_e iqs_show_problem_channels(void);
iqs_error_e iqs_debug_read_raw_counts(void);
iqs_error_e iqs_test_single_channel(void);
iqs_error_e iqs_find_working_channels(void);

iqs_error_e iqs_debug_verify_config(void);
iqs_error_e iqs_debug_test_touch(void);
iqs_error_e iqs_debug_minimal_init(void);
iqs_error_e iqs_debug_skip_ati_init(void);
iqs_error_e iqs_debug_check_hardware_pins(void);
iqs_error_e iqs_run_full_diagnostics(void);
iqs_error_e iqs_fix_and_run_ati(void);
iqs_error_e iqs_init_with_fix(void);

iqs_error_e iqs_test_small_matrix(uint8_t num_tx, uint8_t num_rx, uint8_t *tx_list, uint8_t *rx_list);
iqs_error_e iqs_run_hardware_tests(void);
iqs_error_e iqs_fix_stuck_touch_project56(void);
iqs_error_e iqs_disable_all_channels(void);
iqs_error_e iqs_enable_single_channel(uint8_t tx_num, uint8_t rx_num);
iqs_error_e iqs_enable_specific_channels(uint8_t *tx_list, uint8_t tx_count,
                                          uint8_t *rx_list, uint8_t rx_count);
iqs_error_e iqs_enable_channel_range(uint8_t tx_start, uint8_t tx_end,
                                      uint8_t rx_start, uint8_t rx_end);
iqs_error_e iqs_enable_all_channels(void);
iqs_error_e iqs_run_ati_current_channels(uint16_t ati_target);
iqs_error_e iqs_test_single_channel_ati(uint8_t tx_num, uint8_t rx_num, uint16_t ati_target);
iqs_error_e iqs_scan_all_channels(uint16_t *working_tx_mask, uint16_t *working_rx_mask);
iqs_error_e iqs_quick_channel_test(uint8_t tx, uint8_t rx);
iqs_error_e iqs_init_with_working_channels(uint16_t tx_mask, uint16_t rx_mask);


/*============================================================================
 * ATI Related Register Addresses
 *===========================================================================*/
#define IQS_REG_SYS_CONTROL_0       0x0431
#define IQS_REG_SYS_INFO_0          0x000F

#define IQS_REG_ATI_TARGET          0x056D  // 2 bytes
#define IQS_REG_ALP_ATI_TARGET      0x056F  // 2 bytes
#define IQS_REG_GLOBAL_ATI_C        0x056B  // 1 byte
#define IQS_REG_ALP_ATI_C           0x056C  // 1 byte
#define IQS_REG_REATI_LOWER_LIMIT   0x0573  // 1 byte
#define IQS_REG_REATI_UPPER_LIMIT   0x0574  // 1 byte
#define IQS_REG_MAX_COUNT_LIMIT     0x0575  // 2 bytes

/*============================================================================
 * System Control 0 Bits
 *===========================================================================*/
#define IQS_SYS_CTRL0_ACK_RESET     (1U << 7)
#define IQS_SYS_CTRL0_AUTO_ATI      (1U << 6)
#define IQS_SYS_CTRL0_ALP_RESEED    (1U << 5)
#define IQS_SYS_CTRL0_RESEED        (1U << 4)

/*============================================================================
 * System Info 0 Bits
 *===========================================================================*/
#define IQS_SYS_INFO0_SHOW_RESET    (1U << 7)
#define IQS_SYS_INFO0_ALP_ATI_ERROR (1U << 6)
#define IQS_SYS_INFO0_ATI_ERROR     (1U << 5)
#define IQS_SYS_INFO0_ALP_REATI     (1U << 4)
#define IQS_SYS_INFO0_REATI         (1U << 3)

/*============================================================================
 * Default ATI Configuration Values
 *===========================================================================*/
#define IQS_DEFAULT_ATI_TARGET      500
#define IQS_DEFAULT_ALP_ATI_TARGET  500
#define IQS_DEFAULT_REATI_LOWER     5
#define IQS_DEFAULT_REATI_UPPER     50

/*============================================================================
 * Register Addresses
 *===========================================================================*/
#define IQS_REG_PRODUCT_NUM         0x0000
#define IQS_REG_TOTAL_RX            0x063D
#define IQS_REG_TOTAL_TX            0x063E
#define IQS_REG_RX_MAPPING          0x063F
#define IQS_REG_TX_MAPPING          0x0649
#define IQS_REG_COUNT_VALUES        0x0095  // Raw count values (300 bytes)
#define IQS_REG_ALP_RX_SELECT       0x0659
#define IQS_REG_ALP_TX_SELECT       0x065B
#define IQS_REG_ATI_COMP            0x043F  // ATI compensation (150 bytes)
#define IQS_REG_ATI_TARGET          0x056D
#define IQS_REG_ALP_ATI_TARGET      0x056F
#define IQS_REG_SYS_INFO_0          0x000F


/*============================================================================
 * Register Addresses for Channel Control
 *===========================================================================*/
#define IQS_REG_TOTAL_RX            0x063D
#define IQS_REG_TOTAL_TX            0x063E
#define IQS_REG_RX_MAPPING          0x063F  // 10 bytes
#define IQS_REG_TX_MAPPING          0x0649  // 15 bytes
#define IQS_REG_ATI_TARGET          0x056D  // 2 bytes
#define IQS_REG_SYS_CTRL0           0x0431
#define IQS_REG_SYS_CTRL1           0x0432
#define IQS_REG_SYS_INFO0           0x000F

#endif /* INC_IQS550_TEST_FUNC_H_ */
