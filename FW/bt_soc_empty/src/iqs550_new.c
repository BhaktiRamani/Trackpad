/*
 * iqs550_new.c
 *
 *  Created on: Dec 2, 2025
 *      Author: Bhakti Ramani
 */


#include "../inc/iqs550_new.h"

/**
 * @brief Disable ALP (Alternate Low Power) channel
 */
iqs_error_e iqs5xx_disable_alp(void)
{
    uint8_t data;
    iqs_error_e err = iqs_read_register(0x0658, &data, 1);
    if (err != IQS_OK) return err;

    data &= ~0x10;  /* Clear bit 4 (ALP) - LP1/LP2 use trackpad channels instead */

    return iqs_write_register(0x0658, &data, 1);
}

/**
 * @brief Clear all ALP channel selections
 */
iqs_error_e iqs5xx_clear_alp_channels(void)
{
    uint8_t data[4] = {0x00, 0x00, 0x00, 0x00};

    /* Clear ALP Rx Select (0x0659-0x065A) */
    iqs_error_e err = iqs_write_register(0x0659, data, 2);
    if (err != IQS_OK) return err;

    /* Clear ALP Tx Select (0x065B-0x065C) */
    return iqs_write_register(0x065B, data, 2);
}

/**
 * @brief Manually configure ATI parameters without running AUTO_ATI
 */
iqs_error_e iqs5xx_manual_ati_config(uint16_t target, uint8_t ati_c_global, uint8_t compensation_value)
{
    iqs_error_e err;
    uint8_t data[150];

    /* Set ATI Target */
    data[0] = (target >> 8) & 0xFF;
    data[1] = target & 0xFF;
    err = iqs_write_register(0x056D, data, 2);
    if (err != IQS_OK) return err;

    /* Set Global ATI C */
    data[0] = ati_c_global;
    err = iqs_write_register(0x056B, data, 1);
    if (err != IQS_OK) return err;

    /* Set uniform ATI Compensation for all 150 channels */
    for (int i = 0; i < 150; i++) {
        data[i] = compensation_value;
    }
    err = iqs_write_register(0x043F, data, 150);
    if (err != IQS_OK) return err;

    return IQS_OK;
}

iqs_error_e iqs5xx_configure_total_channels(void)
{
    uint8_t data[2];
    data[0] = 10;  // Total Rx (RxA0-RxA9)
    data[1] = 15;  // Total Tx (Tx0-Tx14)
    return iqs_write_register(0x063D, data, 2);
}

iqs_error_e iqs5xx_configure_channel_mapping(void)
{
    uint8_t rx_map[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};  // Adjust to your layout
    uint8_t tx_map[15] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};

    iqs_error_e err = iqs_write_register(0x063F, rx_map, 10);
    if (err != IQS_OK) return err;

    return iqs_write_register(0x0649, tx_map, 15);
}

iqs_error_e iqs5xx_configure_resolution(uint16_t x_res, uint16_t y_res)
{
    uint8_t data[4];
    data[0] = (x_res >> 8) & 0xFF;
    data[1] = x_res & 0xFF;
    data[2] = (y_res >> 8) & 0xFF;
    data[3] = y_res & 0xFF;
    return iqs_write_register(0x066E, data, 4);
}

iqs_error_e iqs5xx_run_ati(void)
{
    uint8_t data = 0x20;  // Set AUTO_ATI bit
    return iqs_write_register(0x0431, &data, 1);
}

iqs_error_e iqs5xx_setup_complete(void)
{
    uint8_t data;
    iqs_error_e err = iqs_read_register(0x058E, &data, 1);
    if (err != IQS_OK) return err;

    data |= 0x40;  // Set SETUP_COMPLETE bit
    return iqs_write_register(0x058E, &data, 1);
}

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* Register addresses */
#define IQS5XX_SYSTEM_CONTROL_0      0x0431
#define IQS5XX_SYSTEM_INFO_0         0x000F
#define IQS5XX_SYSTEM_INFO_1         0x0010
#define IQS5XX_ATI_COMPENSATION      0x0500
#define IQS5XX_COUNT_VALUES          0x0200
#define IQS5XX_ATI_TARGET            0x0569
#define IQS5XX_ATI_C_GLOBAL          0x056B

/* System Info 0 bit definitions */
#define SHOW_RESET_BIT               (1 << 7)
#define ALP_REATI_OCCURRED_BIT       (1 << 6)
#define ALP_ATI_ERROR_BIT            (1 << 5)
#define REATI_OCCURRED_BIT           (1 << 4)
#define ATI_ERROR_BIT                (1 << 3)

/* Timeout and retry configuration */
#define ATI_TIMEOUT_MS               5000
#define ATI_POLL_INTERVAL_MS         50
#define ATI_MAX_RETRIES              3

/* Debug print macro - disable by defining NDEBUG */
//#ifndef NDEBUG
//    #define LOG_INFO(fmt, ...) printf("[IQS5XX] " fmt "\n", ##__VA_ARGS__)
//#else
//    #define LOG_INFO(fmt, ...)
//#endif

typedef struct {
    uint8_t  system_info_0;
    uint8_t  system_info_1;
    uint16_t ati_target;
    uint8_t  ati_c_global;
    uint16_t sample_count_values[5];   // Sample of first 5 channels
    uint8_t  sample_compensation[5];   // Sample of first 5 channels
} iqs5xx_debug_info_t;

/**
 * @brief Delay function - implement based on your platform
 */
void delay_ms(uint32_t ms){
  sl_udelay_wait(ms);  // 5ms
}

/**
 * @brief Read debug information from device
 */
static iqs_error_e iqs5xx_read_debug_info(iqs5xx_debug_info_t *info)
{
    iqs_error_e err;
    uint8_t data[10];

    /* Read System Info registers */
    err = iqs_read_register(IQS5XX_SYSTEM_INFO_0, &info->system_info_0, 1);
    if (err != IQS_OK) return err;

    err = iqs_read_register(IQS5XX_SYSTEM_INFO_1, &info->system_info_1, 1);
    if (err != IQS_OK) return err;

    /* Read ATI target */
    err = iqs_read_register(IQS5XX_ATI_TARGET, data, 2);
    if (err != IQS_OK) return err;
    info->ati_target = (data[0] << 8) | data[1];

    /* Read Global ATI C */
    err = iqs_read_register(IQS5XX_ATI_C_GLOBAL, &info->ati_c_global, 1);
    if (err != IQS_OK) return err;

    /* Read sample count values (first 5 channels) */
    err = iqs_read_register(IQS5XX_COUNT_VALUES, data, 10);
    if (err != IQS_OK) return err;
    for (int i = 0; i < 5; i++) {
        info->sample_count_values[i] = (data[i * 2] << 8) | data[i * 2 + 1];
    }

    /* Read sample ATI compensation values */
    err = iqs_read_register(IQS5XX_ATI_COMPENSATION, info->sample_compensation, 5);
    if (err != IQS_OK) return err;

    return IQS_OK;
}

/**
 * @brief Print debug information
 */
static void iqs5xx_print_debug_info(const iqs5xx_debug_info_t *info)
{
    LOG_INFO("=== ATI Debug Information ===");
    LOG_INFO("System Info 0: 0x%02X", info->system_info_0);
    LOG_INFO("  - SHOW_RESET:         %d", (info->system_info_0 & SHOW_RESET_BIT) ? 1 : 0);
    LOG_INFO("  - ALP_REATI_OCCURRED: %d", (info->system_info_0 & ALP_REATI_OCCURRED_BIT) ? 1 : 0);
    LOG_INFO("  - ALP_ATI_ERROR:      %d", (info->system_info_0 & ALP_ATI_ERROR_BIT) ? 1 : 0);
    LOG_INFO("  - REATI_OCCURRED:     %d", (info->system_info_0 & REATI_OCCURRED_BIT) ? 1 : 0);
    LOG_INFO("  - ATI_ERROR:          %d", (info->system_info_0 & ATI_ERROR_BIT) ? 1 : 0);
    LOG_INFO("System Info 1: 0x%02X", info->system_info_1);
    LOG_INFO("ATI Target: %u", info->ati_target);
    LOG_INFO("ATI C Global: %u", info->ati_c_global);

    LOG_INFO("Sample Count Values (first 5 channels):");
    for (int i = 0; i < 5; i++) {
        LOG_INFO("  CH[0][%d]: %u (target: %u, diff: %d)",
                    i, info->sample_count_values[i], info->ati_target,
                    (int)info->sample_count_values[i] - (int)info->ati_target);
    }

    LOG_INFO("Sample ATI Compensation (first 5 channels):");
    for (int i = 0; i < 5; i++) {
        LOG_INFO("  CH[0][%d]: %u", i, info->sample_compensation[i]);
    }
    LOG_INFO("=============================");
}

/**
 * @brief Diagnose ATI failure and suggest fixes
 */
static void iqs5xx_diagnose_ati_failure(const iqs5xx_debug_info_t *info)
{
    LOG_INFO("=== ATI Failure Diagnosis ===");

    bool issue_found = false;

    /* Check for reset during ATI */
    if (info->system_info_0 & SHOW_RESET_BIT) {
        LOG_INFO("ISSUE: Device reset occurred - check power supply stability");
        issue_found = true;
    }

    /* Analyze count values */
    for (int i = 0; i < 5; i++) {
        int diff = (int)info->sample_count_values[i] - (int)info->ati_target;

        if (info->sample_count_values[i] < 100) {
            LOG_INFO("ISSUE: CH[0][%d] count very low (%u) - possible open/disconnected electrode",
                        i, info->sample_count_values[i]);
            issue_found = true;
        } else if (info->sample_count_values[i] > 60000) {
            LOG_INFO("ISSUE: CH[0][%d] count very high (%u) - possible short or excessive capacitance",
                        i, info->sample_count_values[i]);
            issue_found = true;
        } else if (diff > 500 || diff < -500) {
            LOG_INFO("WARNING: CH[0][%d] count (%u) far from target (%u)",
                        i, info->sample_count_values[i], info->ati_target);
        }
    }

    /* Check compensation saturation */
    for (int i = 0; i < 5; i++) {
        if (info->sample_compensation[i] == 0) {
            LOG_INFO("ISSUE: CH[0][%d] compensation at minimum - electrode capacitance too high", i);
            LOG_INFO("  -> Try reducing ATI C Global or ATI Target");
            issue_found = true;
        } else if (info->sample_compensation[i] == 255) {
            LOG_INFO("ISSUE: CH[0][%d] compensation at maximum - electrode capacitance too low", i);
            LOG_INFO("  -> Try increasing ATI C Global or ATI Target");
            issue_found = true;
        }
    }

    if (!issue_found) {
        LOG_INFO("No obvious issues found in sampled channels");
        LOG_INFO("Suggestions:");
        LOG_INFO("  - Check all electrode connections");
        LOG_INFO("  - Verify ground plane integrity");
        LOG_INFO("  - Ensure no touch during ATI");
        LOG_INFO("  - Try adjusting ATI target (current: %u)", info->ati_target);
    }

    LOG_INFO("=============================");
}

/**
 * @brief Wait for ATI to complete with timeout
 */
static iqs_error_e iqs5xx_wait_ati_complete(uint32_t timeout_ms)
{
    uint8_t ctrl;
    uint32_t elapsed = 0;

    while (elapsed < timeout_ms) {
        iqs_error_e err = iqs_read_register(IQS5XX_SYSTEM_CONTROL_0, &ctrl, 1);
        if (err != IQS_OK) {
            LOG_INFO("ERROR: Failed to read System Control 0 (err=%d)", err);
            return err;
        }

        /* AUTO_ATI bit clears when complete */
        if ((ctrl & 0x20) == 0) {
            LOG_INFO("ATI completed in %u ms", elapsed);
            return IQS_OK;
        }

        delay_ms(ATI_POLL_INTERVAL_MS);
        elapsed += ATI_POLL_INTERVAL_MS;
    }

    LOG_INFO("ERROR: ATI timeout after %u ms", timeout_ms);
    return IQS_ERROR_TIMEOUT;
}

/**
 * @brief Check for ATI errors
 */
static iqs_error_e iqs5xx_check_ati_error(bool *error_occurred)
{
    uint8_t sys_info;
    iqs_error_e err = iqs_read_register(IQS5XX_SYSTEM_INFO_0, &sys_info, 1);
    if (err != IQS_OK) return err;

    *error_occurred = (sys_info & ATI_ERROR_BIT) != 0;
    return IQS_OK;
}

/**
 * @brief Clear ATI error flag by acknowledging reset
 */
static iqs_error_e iqs5xx_clear_ati_error(void)
{
    uint8_t data = 0x80;  /* ACK_RESET bit */
    return iqs_write_register(IQS5XX_SYSTEM_CONTROL_0, &data, 1);
}

iqs_error_e iqs5xx_dump_all_counts(void)
{
    uint8_t data[300];  /* 150 channels × 2 bytes */

    iqs_error_e err = iqs_read_register(IQS5XX_COUNT_VALUES, data, 300);
    if (err != IQS_OK) {
        LOG_INFO("ERROR: Failed to read count values");
        return err;
    }

    LOG_INFO("=== All Channel Count Values ===");
    LOG_INFO("      Rx0   Rx1   Rx2   Rx3   Rx4   Rx5   Rx6   Rx7   Rx8   Rx9");

    for (int tx = 0; tx < 15; tx++) {
        printf("Tx%2d: ", tx);
        for (int rx = 0; rx < 10; rx++) {
            int idx = (tx * 10 + rx) * 2;
            uint16_t count = (data[idx] << 8) | data[idx + 1];
            printf("%5u ", count);
        }
        printf("\n");
    }

    LOG_INFO("================================");
    return IQS_OK;
}
/**
 * @brief Run ATI with error checking, retry logic, and debugging
 */
iqs_error_e iqs5xx_run_ati_with_check(void)
{
    iqs_error_e err;
    iqs5xx_debug_info_t debug_info;
    bool ati_error;

    for (int attempt = 1; attempt <= ATI_MAX_RETRIES; attempt++) {
        LOG_INFO("Starting ATI attempt %d/%d", attempt, ATI_MAX_RETRIES);

        /* Clear any previous error flags */
        err = iqs5xx_clear_ati_error();
        if (err != IQS_OK) {
            LOG_INFO("WARNING: Failed to clear error flags (err=%d)", err);
        }

        /* Trigger ATI */
        uint8_t data = 0x20;  /* AUTO_ATI bit */
        err = iqs_write_register(IQS5XX_SYSTEM_CONTROL_0, &data, 1);
        if (err != IQS_OK) {
            LOG_INFO("ERROR: Failed to trigger ATI (err=%d)", err);
            return err;
        }

        /* Wait for completion */
        err = iqs5xx_wait_ati_complete(ATI_TIMEOUT_MS);
        if (err != IQS_OK) {
            LOG_INFO("ATI did not complete in time");
            continue;
        }

        /* Check for ATI error */
        err = iqs5xx_check_ati_error(&ati_error);
        if (err != IQS_OK) {
            LOG_INFO("ERROR: Failed to read ATI status (err=%d)", err);
            return err;
        }

        /* Read and print debug info */
        if (iqs5xx_read_debug_info(&debug_info) == IQS_OK) {
            iqs5xx_print_debug_info(&debug_info);
        }

        if (!ati_error) {
            LOG_INFO("ATI completed successfully on attempt %d", attempt);
            return IQS_OK;
        }

        LOG_INFO("ATI ERROR detected on attempt %d", attempt);
        iqs5xx_diagnose_ati_failure(&debug_info);
        iqs5xx_dump_all_counts();
        /* Small delay before retry */
        delay_ms(100);
    }

    LOG_INFO("ATI FAILED after %d attempts", ATI_MAX_RETRIES);
    return IQS_ERROR_ATI_FAILED;
}

/**
 * @brief Full initialization with ATI error handling
 */
iqs_error_e iqs5xx_init(void)
{
    iqs_error_e err;

    LOG_INFO("Starting IQS5xx initialization");

    /* Configure channels */
    err = iqs5xx_configure_total_channels();
    if (err != IQS_OK) {
        LOG_INFO("ERROR: Failed to configure total channels");
        return err;
    }

    err = iqs5xx_configure_channel_mapping();
    if (err != IQS_OK) {
        LOG_INFO("ERROR: Failed to configure channel mapping");
        return err;
    }

    err = iqs5xx_configure_active_channels();
    if (err != IQS_OK) {
        LOG_INFO("ERROR: Failed to configure active channels");
        return err;
    }

    err = iqs5xx_configure_resolution(1920, 1080);
    if (err != IQS_OK) {
        LOG_INFO("ERROR: Failed to configure resolution");
        return err;
    }

    /* Run ATI with error checking */
    err = iqs5xx_run_ati_with_check();
    if (err != IQS_OK) {
        LOG_INFO("ERROR: ATI failed - device may not function correctly");
        return err;
    }

    /* Mark setup complete */
    err = iqs5xx_setup_complete();
    if (err != IQS_OK) {
        LOG_INFO("ERROR: Failed to set SETUP_COMPLETE");
        return err;
    }

    LOG_INFO("IQS5xx initialization complete");
    return IQS_OK;
}

#define IQS5XX_TOTAL_RX              0x063D
#define IQS5XX_TOTAL_TX              0x063E
#define IQS5XX_ACTIVE_CHANNELS       0x067B
#define IQS5XX_SYSTEM_CONFIG_0       0x058E
#define IQS5XX_SYSTEM_CONFIG_1       0x058F
#define IQS5XX_HARDWARE_SETTINGS_A   0x065F
#define IQS5XX_HARDWARE_SETTINGS_B   0x0660
#define IQS5XX_CHARGING_MODE         0x0010  /* Part of System Info 1 */

/**
 * @brief Read and print current device configuration for debugging
 */
iqs_error_e iqs5xx_dump_config(void)
{
    uint8_t data[32];
    iqs_error_e err;

    LOG_INFO("=== Device Configuration Dump ===");

    /* Product info */
    err = iqs_read_register(0x0000, data, 6);
    if (err != IQS_OK) return err;
    LOG_INFO("Product Number: %u", (data[0] << 8) | data[1]);
    LOG_INFO("Project Number: %u", (data[2] << 8) | data[3]);
    LOG_INFO("Version: %u.%u", data[4], data[5]);

    /* Total Rx/Tx */
    err = iqs_read_register(IQS5XX_TOTAL_RX, data, 2);
    if (err != IQS_OK) return err;
    LOG_INFO("Total Rx: %u, Total Tx: %u", data[0], data[1]);

    /* System Config 0 */
    err = iqs_read_register(IQS5XX_SYSTEM_CONFIG_0, data, 1);
    if (err != IQS_OK) return err;
    LOG_INFO("System Config 0: 0x%02X", data[0]);
    LOG_INFO("  - MANUAL_CONTROL:  %d", (data[0] >> 7) & 1);
    LOG_INFO("  - SETUP_COMPLETE:  %d", (data[0] >> 6) & 1);
    LOG_INFO("  - WDT:             %d", (data[0] >> 5) & 1);

    /* System Config 1 */
    err = iqs_read_register(IQS5XX_SYSTEM_CONFIG_1, data, 1);
    if (err != IQS_OK) return err;
    LOG_INFO("System Config 1: 0x%02X", data[0]);
    LOG_INFO("  - EVENT_MODE:      %d", data[0] & 1);

    /* Hardware Settings A */
    err = iqs_read_register(IQS5XX_HARDWARE_SETTINGS_A, data, 1);
    if (err != IQS_OK) return err;
    LOG_INFO("Hardware Settings A: 0x%02X", data[0]);
    LOG_INFO("  - ND (noise det):  %d", (data[0] >> 5) & 1);
    LOG_INFO("  - RX_FLOAT:        %d", (data[0] >> 2) & 1);

    /* Hardware Settings B */
    err = iqs_read_register(IQS5XX_HARDWARE_SETTINGS_B, data, 1);
    if (err != IQS_OK) return err;
    LOG_INFO("Hardware Settings B: 0x%02X", data[0]);
    LOG_INFO("  - CK_FREQ:         %u", (data[0] >> 4) & 0x07);

    /* Active Channels - first row */
    err = iqs_read_register(IQS5XX_ACTIVE_CHANNELS, data, 2);
    if (err != IQS_OK) return err;
    LOG_INFO("Active Channels Row0: 0x%02X%02X", data[0], data[1]);

    /* System Info 0 */
    err = iqs_read_register(0x000F, data, 2);
    if (err != IQS_OK) return err;
    LOG_INFO("System Info 0: 0x%02X", data[0]);
    LOG_INFO("System Info 1: 0x%02X", data[1]);
    LOG_INFO("  - CHARGING_MODE:   %u", data[1] & 0x07);

    LOG_INFO("==================================");
    return IQS_OK;
}
iqs_error_e iqs5xx_end_comm_window()
{
    uint8_t data = 0x00;
    return iqs_write_register(0xEEEE, &data, 1);
}


/**
 * @brief Force a sensing cycle and read fresh counts
 */
iqs_error_e iqs5xx_force_sense_and_read(void)
{
    /* End communication window to trigger new sensing cycle */
    iqs5xx_end_comm_window();

    /* Wait for new data ready */
    delay_ms(100);

    /* Wait for device ready */



    /* Now read counts */
    return iqs5xx_dump_all_counts();
}
#define IQS5XX_SYSTEM_CONTROL_1      0x0432


static iqs_error_e iqs5xx_reset(void)
{
    uint8_t data = 0x02;  /* RESET bit */
    iqs_error_e err = iqs_write_register(IQS5XX_SYSTEM_CONTROL_1, &data, 1);
    if (err != IQS_OK) return err;

    iqs5xx_end_comm_window();
    delay_ms(100);  /* Wait for reset to complete */

    LOG_INFO("Device reset complete");
    return IQS_OK;
}
/**
 * @brief Revised initialization with proper sequencing
 */
iqs_error_e iqs5xx_init_v2(void)
{
    iqs_error_e err;
    uint8_t data[32];

    LOG_INFO("=== IQS5xx Init V2 ===");

    /* Step 1: Wait for device */

    if (err != IQS_OK) return err;

    /* Step 2: Dump initial config */
    LOG_INFO("--- Initial Configuration ---");
    iqs5xx_dump_config();

    /* Step 3: Reset device */
    LOG_INFO("Resetting device...");
    err = iqs5xx_reset();
    if (err != IQS_OK) return err;



    /* Step 4: Acknowledge reset */
    data[0] = 0x80;  /* ACK_RESET */
    err = iqs_write_register(0x0431, data, 1);
    if (err != IQS_OK) return err;
    LOG_INFO("Reset acknowledged");

    /* Step 5: Clear SETUP_COMPLETE to enter config mode */
    err = iqs_read_register(IQS5XX_SYSTEM_CONFIG_0, data, 1);
    if (err != IQS_OK) return err;
    data[0] &= ~0x40;  /* Clear SETUP_COMPLETE */
    err = iqs_write_register(IQS5XX_SYSTEM_CONFIG_0, data, 1);
    if (err != IQS_OK) return err;
    LOG_INFO("SETUP_COMPLETE cleared");

    /* Step 6: Configure Total Rx = 10, Total Tx = 15 */
    data[0] = 10;
    data[1] = 15;
    err = iqs_write_register(IQS5XX_TOTAL_RX, data, 2);
    if (err != IQS_OK) return err;
    LOG_INFO("Total Rx/Tx set to 10/15");

    /* Step 7: Configure Rx mapping (0-9 in order) */
    for (int i = 0; i < 10; i++) {
        data[i] = i;
    }
    err = iqs_write_register(0x063F, data, 10);
    if (err != IQS_OK) return err;
    LOG_INFO("Rx mapping configured");

    /* Step 8: Configure Tx mapping (0-14 in order) */
    for (int i = 0; i < 15; i++) {
        data[i] = i;
    }
    err = iqs_write_register(0x0649, data, 15);
    if (err != IQS_OK) return err;
    LOG_INFO("Tx mapping configured");

    /* Step 9: Configure Active Channels - enable Rx0-Rx9 for all Tx rows */
    for (int tx = 0; tx < 15; tx++) {
        data[tx * 2]     = 0x03;  /* High byte: bits 9,8 set */
        data[tx * 2 + 1] = 0xFF;  /* Low byte: bits 7-0 set */
    }
    err = iqs_write_register(IQS5XX_ACTIVE_CHANNELS, data, 30);
    if (err != IQS_OK) return err;
    LOG_INFO("Active channels configured (all 150 channels enabled)");

    /* Step 10: Configure resolution */
    data[0] = 0x07;  /* 1920 high byte */
    data[1] = 0x80;  /* 1920 low byte */
    data[2] = 0x04;  /* 1080 high byte */
    data[3] = 0x38;  /* 1080 low byte */
    err = iqs_write_register(0x066E, data, 4);
    if (err != IQS_OK) return err;
    LOG_INFO("Resolution set to 1920x1080");

    /* Step 11: End window to apply settings */
    iqs5xx_end_comm_window();
    delay_ms(100);

    /* Step 12: Wait and verify */


    LOG_INFO("--- Configuration After Setup ---");
    iqs5xx_dump_config();

    /* Step 13: Force sense cycle and check counts BEFORE ATI */
    LOG_INFO("--- Counts Before ATI ---");
    iqs5xx_force_sense_and_read();

    /* Step 14: Run ATI */
    LOG_INFO("Running ATI...");
    err = iqs5xx_run_ati_with_check();
    if (err != IQS_OK) {
        LOG_INFO("ATI failed - checking counts after failure");
        iqs5xx_force_sense_and_read();
        return err;
    }

    /* Step 15: Set SETUP_COMPLETE */
    err = iqs_read_register(IQS5XX_SYSTEM_CONFIG_0, data, 1);
    if (err != IQS_OK) return err;
    data[0] |= 0x40;  /* Set SETUP_COMPLETE */
    err = iqs_write_register(IQS5XX_SYSTEM_CONFIG_0, data, 1);
    if (err != IQS_OK) return err;

    iqs5xx_end_comm_window();
    delay_ms(50);

    /* Step 16: Final count check */

    if (err != IQS_OK) return err;

    LOG_INFO("--- Final Counts ---");
    iqs5xx_dump_all_counts();

    LOG_INFO("=== Initialization Complete ===");
    return IQS_OK;
}
