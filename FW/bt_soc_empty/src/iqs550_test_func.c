/*
 * iqs550_test_func.c
 *
 *  Created on: Nov 28, 2025
 *      Author: Bhakti Ramani
 */

#include "../inc/iqs550_test_func.h"




iqs_error_e iqs_auto_ATI_with_retry(void)
{
    const uint8_t MAX_ATTEMPTS = 6;

    for (uint8_t attempt = 1; attempt <= MAX_ATTEMPTS; attempt++)
    {
        LOG_INFO("=== Auto ATI attempt %d/%d ===", attempt, MAX_ATTEMPTS);

        // ------------------------------------------------------------------
        // 1. Force sane ATI parameters (THESE ARE THE MAGIC NUMBERS)
        // ------------------------------------------------------------------
        // ATI Target (0x056D)          → 400
//        ALP ATI Target (0x056F)       → 400
//        Max Count Limit (0x0575)      → 600
//        Global ATI Compensation (0x056B) → 32
//        ALP ATI Compensation (0x056C)    → 32
//        Re-ATI Lower Limit (0x0573)   → 8
//        Re-ATI Upper Limit (0x0574)   → 40

        uint16_t val16;
        uint8_t  val8;

        val16 = 400;  iqs_write_register(0x056D, &val16, 2);  // ATI Target
        val16 = 400;  iqs_write_register(0x056F, &val16, 2);  // ALP ATI Target
        val16 = 600;  iqs_write_register(0x0575, &val16, 2);  // Max Count Limit ← critical!
        val8  = 32;   iqs_write_register (0x056B, &val8, 1);   // Global ATI C
        val8  = 32;   iqs_write_register (0x056C, &val8, 1);   // ALP ATI C
        val8  = 8;    iqs_write_register (0x0573, &val8, 1);   // Re-ATI Lower
        val8  = 40;   iqs_write_register (0x0574, &val8, 1);   // Re-ATI Upper ← this was failing, but now safe

        // ------------------------------------------------------------------
        // 2. Trigger Re-ATI the correct way
        // ------------------------------------------------------------------
        uint16_t bit = 0x0002;
        iqs_write_register(0x0430, &bit, 2);    // Set bit1 = RE_ATI
        sl_udelay_wait(5000);
        bit = 0x0000;
        iqs_write_register(0x0430, &bit, 2);    // Clear

        // ------------------------------------------------------------------
        // 3. Wait for ATI to finish (max 800 ms is more than enough)
        // ------------------------------------------------------------------
        uint32_t timeout = 800;
        uint16_t ati_status;
        while (timeout--)
        {
            if (iqs_read_register(0x0010, &ati_status, 2) == IQS_OK)
            {
                if ((ati_status & 0x0001) == 0)  // ATI not busy
                {
                    if (ati_status & 0x0002)     // ATI error bit
                    {
                        LOG_WARN("ATI failed this attempt, retrying...");
                        break;  // will retry whole loop
                    }
                    LOG_INFO("ATI SUCCESS on attempt %d!", attempt);
                    return IQS_OK;
                }
            }
            sl_udelay_wait(1000);
        }
        sl_udelay_wait(500000);  // 500ms delay
    }

    LOG_ERR("ATI failed after %d attempts — hardware issue or bad sensor", MAX_ATTEMPTS);
    return IQS_ERR_TIMEOUT;
}
#define MAX_ATI_RETRIES   5          // Number of retries before giving up
#define ATI_TARGET_VALUE  512        // Your ATI target

iqs_error_e iqs_auto_ATI3(void)
{
    iqs_error_e err;
    uint8_t buf[2];
    uint8_t sys_info = 0;
    uint8_t ati_status = 0;

    LOG_INFO("Starting AUTO-ATI with retry mechanism...");

    for (int attempt = 1; attempt <= MAX_ATI_RETRIES; attempt++)
    {
        LOG_WARN("=== AUTO-ATI Attempt %d/%d ===", attempt, MAX_ATI_RETRIES);

        //
        // 1) Write ATI_TARGET (big-endian)
        //
        buf[0] = (ATI_TARGET_VALUE >> 8) & 0xFF;   // MSB
        buf[1] = ATI_TARGET_VALUE & 0xFF;          // LSB

        err = iqs_write_register(IQS_REG_ATI_TARGET, buf, 2);
        if (err != IQS_OK) {
            LOG_ERR("Failed to write ATI_TARGET: %s", iqs_error_to_str(err));
            return err;
        }

        //
        // 2) Read back ATI_TARGET (big-endian interpretation)
        //
        err = iqs_read_register(IQS_REG_ATI_TARGET, buf, 2);
        if (err != IQS_OK) {
            LOG_ERR("Failed to read ATI_TARGET: %s", iqs_error_to_str(err));
            return err;
        }

        uint16_t readback = ((uint16_t)buf[0] << 8) | buf[1];
        LOG_INFO("ATI_TARGET readback: 0x%04X (%d)", readback, readback);

        //
        // 3) Enable REATI + ALP_REATI
        //
        uint8_t sys_config_flag = IQS_SYS_CONFIG_0_REATI |
                                  IQS_SYS_CONFIG_0_ALP_REATI;

        err = iqs_write_register(IQS_REG_SYS_CONFIG_0, &sys_config_flag, 1);
        if (err != IQS_OK) {
            LOG_ERR("Failed config REATI bits: %s", iqs_error_to_str(err));
            return err;
        }

        //
        // 4) Trigger AUTO_ATI
        //
        uint8_t sys_control_flag = IQS_SYS_CONTROL_0_AUTO_ATI;

        err = iqs_write_register(IQS_REG_SYS_CONTROL_0, &sys_control_flag, 1);
        if (err != IQS_OK) {
            LOG_ERR("Failed to set AUTO_ATI bit: %s", iqs_error_to_str(err));
            return err;
        }

        //
        // 5) Wait for AUTO_ATI to complete
        //
        do {
            sl_udelay_wait(1500);  // 1.5 ms
            err = iqs_read_register(IQS_REG_SYS_CONTROL_0, &ati_status, 1);
            if (err != IQS_OK) {
                LOG_ERR("Failed reading AUTO_ATI bit: %s", iqs_error_to_str(err));
                return err;
            }
        } while (ati_status & IQS_SYS_CONTROL_0_AUTO_ATI);

        //
        // 6) Check ATI_ERROR flag
        //
        err = iqs_read_register(IQS_REG_SYSTEM_INFO_0, &sys_info, 1);
        if (err != IQS_OK) {
            LOG_ERR("Failed to read SYSTEM_INFO_0: %s", iqs_error_to_str(err));
            return err;
        }

        if (!(sys_info & (IQS_SYS_INFO_0_ATI_ERROR | IQS_SYS_INFO_0_ALP_ATI_ERROR)))
        {
            // 🎉 SUCCESS — No error flag
            LOG_INFO("AUTO-ATI SUCCESS on attempt %d!", attempt);
            return IQS_OK;
        }

        //
        // 7) ATI_ERROR — Retry Needed
        //
        LOG_ERR("AUTO-ATI FAILED on attempt %d, sys_info=0x%02X", attempt, sys_info);
    }

    // ❌ All attempts failed
    LOG_ERR("AUTO-ATI FAILED after %d attempts. Giving up.", MAX_ATI_RETRIES);
    return IQS_ERR_NOT_INITIALIZED;
}
int iqs_end_communication_window(void)
{
    uint8_t dummy = 0x00;

    // Write any byte to 0xEEEE to end the communication window
    int err = iqs_write_register(0xEEEE, &dummy, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to end communication window");
        return err;
    }

    LOG_DBG("Communication window ended");
    return IQS_OK;
}

bool iqs_wait_for_rdy(uint32_t timeout_ms)
{
//    uint32_t start = k_uptime_get_32();
//
//    while ((k_uptime_get_32() - start) < timeout_ms) {
//        if (gpio_pin_get_dt(TRACKPAD_MCU_RDY_PIN) == 1) {
//            return true;
//        }
//        //////k_msleep(10);
//    }

    return true;
}

// Add these helper functions:
static inline uint16_t iqs_read_u16_be(const uint8_t *buf)
{
    return (buf[0] << 8) | buf[1];  // Big endian
}

static inline void iqs_write_u16_be(uint16_t value, uint8_t *buf)
{
    buf[0] = (value >> 8) & 0xFF;  // MSB first
    buf[1] = value & 0xFF;         // LSB
}

int iqs_auto_ATI4(void)
{
    int err;
    uint8_t buf[2];

    LOG_INFO("Configuring AUTO ATI...");

    // === CRITICAL: All writes must happen BEFORE triggering AUTO_ATI ===

    // 1. First, disable Re-ATI to prevent automatic changes
    uint8_t sys_config;
    err = iqs_read_register(0x058E, &sys_config, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read System Config 0");
        return err;
    }

    sys_config &= ~0x02;  // Clear REATI bit
    err = iqs_write_register(0x058E, &sys_config, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to disable REATI");
        return err;
    }

    // 2. Write ATI Target (Big Endian!)
    uint16_t ati_target = 1000;  // Try 1000 first
    iqs_write_u16_be(ati_target, buf);
    err = iqs_write_register(0x056E, buf, 2);
    if (err != IQS_OK) {
        LOG_ERR("Failed to write ATI_TARGET");
        return err;
    }
    LOG_INFO("Wrote ATI_TARGET: %u (0x%04X)", ati_target, ati_target);

    // 3. Write Global ATI C to maximum
    uint8_t global_ati_c = 0xFF;  // Max value
    err = iqs_write_register(0x056B, &global_ati_c, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to write GLOBAL_ATI_C");
        return err;
    }
    LOG_INFO("Wrote GLOBAL_ATI_C: %u", global_ati_c);

    // 4. Set compensation limits
    uint8_t comp_lower = 20;
    uint8_t comp_upper = 230;
    err = iqs_write_register(0x0573, &comp_lower, 1);
    err |= iqs_write_register(0x0574, &comp_upper, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to write compensation limits");
        return err;
    }

    // 5. Set max count limit (big endian!)
    uint16_t max_count = 0x3000;  // 12288
    iqs_write_u16_be(max_count, buf);
    err = iqs_write_register(0x0576, buf, 2);
    if (err != IQS_OK) {
        LOG_ERR("Failed to write MAX_COUNT_LIMIT");
        return err;
    }

    // 6. END COMMUNICATION WINDOW (CRITICAL!)
    err = iqs_end_communication_window();
    if (err != IQS_OK) {
        LOG_ERR("Failed to end comm window before ATI");
        return err;
    }

    // 7. Wait for next communication window
//    //////k_msleep(50);

    // 8. NOW trigger Auto ATI
    uint8_t sys_ctrl = 0x04;  // Set AUTO_ATI bit
    err = iqs_write_register(0x0431, &sys_ctrl, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to trigger AUTO_ATI");
        return err;
    }
    LOG_INFO("AUTO_ATI triggered");

    // 9. END COMMUNICATION WINDOW to let ATI run
    err = iqs_end_communication_window();
    if (err != IQS_OK) {
        LOG_ERR("Failed to end comm window after ATI trigger");
        return err;
    }

    // 10. Wait for ATI to complete (can take 100-500ms)
//    //////k_msleep(500);

    // 11. Wait for RDY to go high indicating new data


    // 12. Check if ATI completed successfully
    err = iqs_read_register(0x0431, &sys_ctrl, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read System Control 0");
        return err;
    }

    if (sys_ctrl & 0x04) {
        LOG_WARN("AUTO_ATI bit still set - ATI may not be complete");
    } else {
        LOG_INFO("AUTO_ATI bit cleared - ATI complete");
    }

    // 13. Check for ATI errors
    uint8_t sys_info;
    err = iqs_read_register(0x000F, &sys_info, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read System Info 0");
        return err;
    }

    if (sys_info & 0x10) {
        LOG_ERR("ATI_ERROR flag set! sys_info=0x%02X", sys_info);

        // Read back what actually got configured
        err = iqs_read_register(0x056E, buf, 2);
        uint16_t actual_target = iqs_read_u16_be(buf);
        LOG_ERR("Actual ATI_TARGET: %u (0x%04X)", actual_target, actual_target);

        return IQS_ERR_NOT_INITIALIZED;
    }

    // 14. Read back actual values (use BIG ENDIAN!)
    err = iqs_read_register(0x056E, buf, 2);
    uint16_t readback_target = iqs_read_u16_be(buf);  // Big endian!
    LOG_INFO("ATI_TARGET readback: %u (0x%04X)", readback_target, readback_target);

    err = iqs_read_register(0x056B, &global_ati_c, 1);
    LOG_INFO("GLOBAL_ATI_C readback: %u", global_ati_c);

    // 15. Check a few compensation values
    uint8_t comp[10];
    err = iqs_read_register(0x043F, comp, 10);
    if (err == IQS_OK) {
        LOG_INFO("First 10 ATI compensation values:");
        for (int i = 0; i < 10; i++) {
            LOG_INFO("  Channel %d: %u", i, comp[i]);
        }
    }

    // 16. End communication window
    iqs_end_communication_window();

    LOG_INFO("AUTO ATI configuration complete");
    return IQS_OK;
}

int iqs_sys_config2(void)
{
    int err;

    LOG_INFO("Configuring system...");


    // Write System Config 0 (0x058E)
    uint8_t sys_config_0 = 0x00;
    sys_config_0 |= 0x40;  // SETUP_COMPLETE
    sys_config_0 |= 0x02;  // REATI (re-enable after ATI is done)
    // Note: Don't set SW_INPUT_EVENT, ALP_REATI unless you need them

    err = iqs_write_register(0x058E, &sys_config_0, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to write System Config 0");
        return err;
    }

    // Write System Config 1 (0x058F) - This is the event mode register
    uint8_t sys_config_1 = 0x00;
    sys_config_1 |= 0x01;  // EVENT_MODE
    sys_config_1 |= 0x02;  // GESTURE_EVENT
    sys_config_1 |= 0x04;  // TP_EVENT
    sys_config_1 |= 0x80;  // PROX_EVENT

    err = iqs_write_register(0x058F, &sys_config_1, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to write System Config 1");
        return err;
    }

    LOG_INFO("System config written: Config0=0x%02X, Config1=0x%02X",
             sys_config_0, sys_config_1);

    // End communication window
    iqs_end_communication_window();


    // Read back to verify
    err = iqs_read_register(0x058F, &sys_config_1, 1);
    if (err == IQS_OK) {
        LOG_INFO("System Config 1 readback: 0x%02X", sys_config_1);
        if (sys_config_1 & 0x01) {
            LOG_INFO("EVENT_MODE enabled successfully");
        } else {
            LOG_ERR("EVENT_MODE not set!");
        }
    }

    iqs_end_communication_window();

    return IQS_OK;
}

// Helper macro for binary printing
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0')
int iqs_configure_active_channels(void)
{
    int err;
    uint8_t buf[30];  // 30 bytes for Active channels register

    LOG_INFO("Configuring active channels...");

    // Wait for RDY
    if (!iqs_wait_for_rdy(2000)) {
        LOG_ERR("Timeout waiting for RDY");
        return IQS_ERR_TIMEOUT;
    }

    // Read current Total Rx/Tx
    uint8_t total_rx, total_tx;
    err = iqs_read_register(0x063D, &total_rx, 1);
    err |= iqs_read_register(0x063E, &total_tx, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read Total Rx/Tx");
        return err;
    }
    LOG_INFO("Total Rx: %d, Total Tx: %d", total_rx, total_tx);

    // Read current active channels (0x067B - 0x0698 = 30 bytes)
    err = iqs_read_register(0x067B, buf, 30);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read active channels");
        return err;
    }

    LOG_INFO("Current active channels (first 10 bytes):");
    for (int i = 0; i < 10; i++) {
        LOG_INFO("  Byte %d: 0b" BYTE_TO_BINARY_PATTERN, i, BYTE_TO_BINARY(buf[i]));
    }

    // Configure: Enable only RxA pins (even channels)
    // Each byte has 8 bits representing 8 channels
    // Bit pattern: Enable even channels (0,2,4,6,8,10,12,14)
    //              Disable odd channels (1,3,5,7,9,11,13,15)

    memset(buf, 0, 30);  // Start with all disabled

    // For each Tx (row)
    for (int tx = 0; tx < total_tx; tx++) {
        // For each Rx (column) - enable only even Rx (RxA pins)
        for (int rx = 0; rx < total_rx; rx += 2) {  // Only even: 0,2,4,6,8...
            int channel = (tx * total_rx) + rx;
            int byte_idx = channel / 8;
            int bit_idx = channel % 8;

            if (byte_idx < 30) {
                buf[byte_idx] |= (1 << bit_idx);
            }
        }
    }

    LOG_INFO("Writing new active channels configuration...");
    err = iqs_write_register(0x067B, buf, 30);
    if (err != IQS_OK) {
        LOG_ERR("Failed to write active channels");
        return err;
    }

    // End communication window
    iqs_end_communication_window();
//    //k_msleep(50);

    // Wait for RDY and verify
    if (!iqs_wait_for_rdy(2000)) {
        LOG_WARN("No RDY after channel config");
    }

    // Read back to verify
    err = iqs_read_register(0x067B, buf, 30);
    if (err == IQS_OK) {
        LOG_INFO("Active channels readback (first 10 bytes):");
        for (int i = 0; i < 10; i++) {
            LOG_INFO("  Byte %d: 0b" BYTE_TO_BINARY_PATTERN, i, BYTE_TO_BINARY(buf[i]));
        }
    }

    iqs_end_communication_window();

    LOG_INFO("Active channels configured - re-run ATI now");
    return IQS_OK;
}

int iqs_auto_ATI_with_device_defaults(void)
{
    int err;
    uint8_t buf[2];

    LOG_INFO("Running AUTO ATI with device defaults...");

    // 1. DON'T write ATI_TARGET - let device use its default (0x1A00 = 6656)
    // 2. DON'T write GLOBAL_ATI_C - let device use its default (26)

    // But DO configure the compensation limits to be more permissive
    uint8_t comp_lower = 10;   // Allow very low compensation
    uint8_t comp_upper = 240;  // Allow very high compensation

    err = iqs_write_register(0x0573, &comp_lower, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to write compensation lower limit");
        return err;
    }

    err = iqs_write_register(0x0574, &comp_upper, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to write compensation upper limit");
        return err;
    }

    // Set higher max count limit for your high target
    uint16_t max_count = 0x4000;  // 16384 - well above 6656
    iqs_write_u16_be(max_count, buf);
    err = iqs_write_register(0x0576, buf, 2);
    if (err != IQS_OK) {
        LOG_ERR("Failed to write max count limit");
        return err;
    }

    // Disable Re-ATI temporarily
    uint8_t sys_config_0;
    err = iqs_read_register(0x058E, &sys_config_0, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read System Config 0");
        return err;
    }

    sys_config_0 &= ~0x02;  // Clear REATI bit
    err = iqs_write_register(0x058E, &sys_config_0, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to disable REATI");
        return err;
    }

    // End communication window
    iqs_end_communication_window();
    //////k_msleep(50);

    // Wait for RDY
    if (!iqs_wait_for_rdy(2000)) {
        LOG_ERR("Timeout waiting for RDY before ATI trigger");
        return IQS_ERR_TIMEOUT;
    }

    // Trigger Auto ATI
    uint8_t sys_ctrl = 0x04;  // AUTO_ATI bit
    err = iqs_write_register(0x0431, &sys_ctrl, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to trigger AUTO_ATI");
        return err;
    }
    LOG_INFO("AUTO_ATI triggered");

    // End communication window to let ATI run
    iqs_end_communication_window();

    // Wait longer for ATI to complete with high target
    //////k_msleep(1000);

    // Wait for RDY
    if (!iqs_wait_for_rdy(5000)) {
        LOG_ERR("Timeout waiting for RDY after ATI");
        return IQS_ERR_TIMEOUT;
    }

    // Check ATI completion
    err = iqs_read_register(0x0431, &sys_ctrl, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read System Control 0");
        return err;
    }

    if (sys_ctrl & 0x04) {
        LOG_WARN("AUTO_ATI bit still set");
    } else {
        LOG_INFO("AUTO_ATI completed");
    }

    // Check for errors
    uint8_t sys_info;
    err = iqs_read_register(0x000F, &sys_info, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read System Info 0");
        return err;
    }

    LOG_INFO("System Info 0: 0x%02X", sys_info);

    if (sys_info & 0x10) {
        LOG_ERR("ATI_ERROR flag is set!");

        // Check individual compensation values to see what failed
        uint8_t comp[150];
        err = iqs_read_register(0x043F, comp, 150);
        if (err == IQS_OK) {
            int extreme_low = 0, extreme_high = 0, good = 0;
            for (int i = 0; i < 150; i++) {
                if (comp[i] < 20) extreme_low++;
                else if (comp[i] > 235) extreme_high++;
                else good++;
            }
            LOG_ERR("Compensation analysis: %d good, %d too low, %d too high",
                    good, extreme_low, extreme_high);
        }

        return IQS_ERR_NOT_INITIALIZED;
    }

    // Read actual values
    err = iqs_read_register(0x056E, buf, 2);
    uint16_t ati_target = iqs_read_u16_be(buf);

    uint8_t ati_c;
    err = iqs_read_register(0x056B, &ati_c, 1);

    LOG_INFO("Final ATI_TARGET: %u (0x%04X)", ati_target, ati_target);
    LOG_INFO("Final GLOBAL_ATI_C: %u", ati_c);

    // Check compensation values
    uint8_t comp[20];
    err = iqs_read_register(0x043F, comp, 20);
    if (err == IQS_OK) {
        LOG_INFO("First 20 ATI compensation values:");
        for (int i = 0; i < 20; i++) {
            LOG_INFO("  Ch %d: %u", i, comp[i]);
        }
    }

    iqs_end_communication_window();

    return IQS_OK;
}

/**
 * @brief Read and display all ATI-related parameters
 * @return IQS_OK on success, error code otherwise
 */
int iqs_read_ati_parameters(void)
{
    int err;
    uint8_t buf[300];

    LOG_INFO("╔═══════════════════════════════════════════════════════════╗");
    LOG_INFO("║           IQS550 ATI PARAMETERS REPORT                    ║");
    LOG_INFO("╚═══════════════════════════════════════════════════════════╝");

    // Wait for RDY
    if (!iqs_wait_for_rdy(2000)) {
        LOG_ERR("Timeout waiting for RDY");
        return IQS_ERR_TIMEOUT;
    }

    // ========== SECTION 1: ATI Target Values ==========
    LOG_INFO("\n┌─── ATI Target Values ───────────────────────────────────┐");

    // ATI Target (0x056E-0x056F)
    err = iqs_read_register(0x056E, buf, 2);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read ATI_TARGET");
        return err;
    }
    uint16_t ati_target = iqs_read_u16_be(buf);
    LOG_INFO("│ ATI Target (0x056E):           %5u (0x%04X)          │",
             ati_target, ati_target);

    // ALP ATI Target (0x0570-0x0571)
    err = iqs_read_register(0x0570, buf, 2);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read ALP_ATI_TARGET");
        return err;
    }
    uint16_t alp_ati_target = iqs_read_u16_be(buf);
    LOG_INFO("│ ALP ATI Target (0x0570):       %5u (0x%04X)          │",
             alp_ati_target, alp_ati_target);

    LOG_INFO("└─────────────────────────────────────────────────────────┘");

    // ========== SECTION 2: ATI C Values ==========
    LOG_INFO("\n┌─── ATI C (Multiplier) Values ───────────────────────────┐");

    // Global ATI C (0x056B)
    err = iqs_read_register(0x056B, buf, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read GLOBAL_ATI_C");
        return err;
    }
    uint8_t global_ati_c = buf[0];
    LOG_INFO("│ Global ATI C (0x056B):         %3u (0x%02X)              │",
             global_ati_c, global_ati_c);

    if (global_ati_c < 50) {
        LOG_WARN("│   ⚠ WARNING: Very low! May cause ATI failure           │");
    } else if (global_ati_c > 200) {
        LOG_INFO("│   ✓ High value - good for low capacitance sensors      │");
    } else {
        LOG_INFO("│   ✓ Normal range                                       │");
    }

    // ALP ATI C (0x056C)
    err = iqs_read_register(0x056C, buf, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read ALP_ATI_C");
        return err;
    }
    uint8_t alp_ati_c = buf[0];
    LOG_INFO("│ ALP ATI C (0x056C):            %3u (0x%02X)              │",
             alp_ati_c, alp_ati_c);

    LOG_INFO("└─────────────────────────────────────────────────────────┘");

    // ========== SECTION 3: ATI Compensation Limits ==========
    LOG_INFO("\n┌─── ATI Compensation Limits ─────────────────────────────┐");

    // Re-ATI lower compensation limit (0x0573)
    err = iqs_read_register(0x0573, buf, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read compensation limits");
        return err;
    }
    uint8_t comp_lower = buf[0];

    // Re-ATI upper compensation limit (0x0574)
    err = iqs_read_register(0x0574, buf, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read compensation limits");
        return err;
    }
    uint8_t comp_upper = buf[0];

    LOG_INFO("│ Lower Compensation Limit:      %3u                       │", comp_lower);
    LOG_INFO("│ Upper Compensation Limit:      %3u                       │", comp_upper);
    LOG_INFO("│ Valid Range:                   [%3u - %3u]               │",
             comp_lower, comp_upper);

    if (comp_upper - comp_lower < 100) {
        LOG_WARN("│   ⚠ WARNING: Narrow range may cause ATI errors         │");
    }

    LOG_INFO("└─────────────────────────────────────────────────────────┘");

    // ========== SECTION 4: Drift and Count Limits ==========
    LOG_INFO("\n┌─── Drift & Count Limits ────────────────────────────────┐");

    // Reference drift limit (0x0571)
    err = iqs_read_register(0x0571, buf, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read reference drift limit");
        return err;
    }
    uint8_t ref_drift = buf[0];
    LOG_INFO("│ Reference Drift Limit:         %3u                       │", ref_drift);
    LOG_INFO("│   Re-ATI triggers when: |ref - target| > %u              │", ref_drift);

    // ALP LTA drift limit (0x0572)
    err = iqs_read_register(0x0572, buf, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read ALP LTA drift limit");
        return err;
    }
    uint8_t alp_drift = buf[0];
    LOG_INFO("│ ALP LTA Drift Limit:           %3u                       │", alp_drift);

    // Max count limit (0x0576-0x0577)
    err = iqs_read_register(0x0576, buf, 2);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read max count limit");
        return err;
    }
    uint16_t max_count = iqs_read_u16_be(buf);
    LOG_INFO("│ Max Count Limit (0x0576):      %5u (0x%04X)          │",
             max_count, max_count);

    if (max_count < ati_target) {
        LOG_ERR("│   ✗ ERROR: Max count < ATI target!                     │");
    }

    // Re-ATI retry time (0x0577)
    err = iqs_read_register(0x0577, buf, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read Re-ATI retry time");
        return err;
    }
    uint8_t reati_retry = buf[0];
    LOG_INFO("│ Re-ATI Retry Time:             %3u seconds              │", reati_retry);

    // Minimum count Re-ATI delta (0x062E)
    err = iqs_read_register(0x062E, buf, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read min count Re-ATI delta");
        return err;
    }
    uint8_t min_count_delta = buf[0];
    LOG_INFO("│ Min Count Re-ATI Delta:        %3u                       │", min_count_delta);

    LOG_INFO("└─────────────────────────────────────────────────────────┘");

    // ========== SECTION 5: ATI Compensation Values (Sample) ==========
    LOG_INFO("\n┌─── ATI Compensation Values (First 30 Channels) ─────────┐");

    // Read ATI compensation values (0x043F onwards, 150 bytes total)
    err = iqs_read_register(0x043F, buf, 30);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read ATI compensation values");
        return err;
    }

    int at_zero = 0, at_max = 0, good = 0, marginal = 0;

    for (int i = 0; i < 30; i++) {
        const char *status;
        if (buf[i] == 0) {
            status = "FAIL-LOW";
            at_zero++;
        } else if (buf[i] == 255) {
            status = "FAIL-HIGH";
            at_max++;
        } else if (buf[i] < comp_lower) {
            status = "Below limit";
            marginal++;
        } else if (buf[i] > comp_upper) {
            status = "Above limit";
            marginal++;
        } else if (buf[i] < 50 || buf[i] > 200) {
            status = "Marginal";
            marginal++;
        } else {
            status = "Good";
            good++;
        }

        LOG_INFO("│ Channel %2d:  %3u  %-12s                      │",
                 i, buf[i], status);
    }

    LOG_INFO("├─────────────────────────────────────────────────────────┤");
    LOG_INFO("│ Summary (first 30 channels):                            │");
    LOG_INFO("│   Good:           %2d                                     │", good);
    LOG_INFO("│   Marginal:       %2d                                     │", marginal);
    LOG_INFO("│   At Zero:        %2d  (ATI failed - too low)            │", at_zero);
    LOG_INFO("│   At Max (255):   %2d  (ATI failed - too high)           │", at_max);
    LOG_INFO("└─────────────────────────────────────────────────────────┘");

    // ========== SECTION 6: ALP Compensation Values ==========
    LOG_INFO("\n┌─── ALP ATI Compensation Values ─────────────────────────┐");

    // Read ALP compensation values (0x0431 onwards, 10 bytes)
    err = iqs_read_register(0x0431, buf, 10);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read ALP ATI compensation");
        return err;
    }

    for (int i = 0; i < 10; i++) {
        if (buf[i] > 0) {  // Only show if used
            LOG_INFO("│ ALP Rx %d:     %3u                                     │",
                     i, buf[i]);
        }
    }

    LOG_INFO("└─────────────────────────────────────────────────────────┘");

    // ========== SECTION 7: Individual ATI C Adjustments (Sample) ==========
    LOG_INFO("\n┌─── Individual ATI C Adjustments (First 20 Channels) ───┐");

    // Read individual ATI C adjustments (0x04D5 onwards, signed values)
    err = iqs_read_register(0x04D5, buf, 20);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read individual ATI C adjustments");
        return err;
    }

    int non_zero_adjustments = 0;
    for (int i = 0; i < 20; i++) {
        int8_t adjustment = (int8_t)buf[i];  // Interpret as signed
        if (adjustment != 0) {
            non_zero_adjustments++;
            uint8_t effective = (uint8_t)((int16_t)global_ati_c + adjustment);
            LOG_INFO("│ Ch %2d:  %+4d  (Effective ATI C: %3u)                 │",
                     i, adjustment, effective);
        }
    }

    if (non_zero_adjustments == 0) {
        LOG_INFO("│ All channels use Global ATI C (no adjustments)          │");
    }

    LOG_INFO("└─────────────────────────────────────────────────────────┘");

    // ========== SECTION 8: System Status ==========
    LOG_INFO("\n┌─── ATI System Status ───────────────────────────────────┐");

    // Read System Info 0 (0x000F)
    err = iqs_read_register(0x000F, buf, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read System Info 0");
        return err;
    }
    uint8_t sys_info_0 = buf[0];

    bool ati_error = (sys_info_0 & 0x10) != 0;
    bool alp_ati_error = (sys_info_0 & 0x20) != 0;
    bool reati_occurred = (sys_info_0 & 0x08) != 0;
    bool alp_reati_occurred = (sys_info_0 & 0x40) != 0;

    LOG_INFO("│ System Info 0 (0x000F):        0x%02X                     │", sys_info_0);
    LOG_INFO("│   ATI_ERROR:                   %s                        │",
             ati_error ? "SET (FAILED!)" : "Clear (OK)");
    LOG_INFO("│   ALP_ATI_ERROR:               %s                        │",
             alp_ati_error ? "SET (FAILED!)" : "Clear (OK)");
    LOG_INFO("│   REATI_OCCURRED:              %s                        │",
             reati_occurred ? "Yes" : "No");
    LOG_INFO("│   ALP_REATI_OCCURRED:          %s                        │",
             alp_reati_occurred ? "Yes" : "No");

    // Read System Config 0 (0x058E) to check if REATI is enabled
    err = iqs_read_register(0x058E, buf, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read System Config 0");
        return err;
    }
    bool reati_enabled = (buf[0] & 0x02) != 0;
    bool alp_reati_enabled = (buf[0] & 0x04) != 0;

    LOG_INFO("│   REATI Enabled:               %s                        │",
             reati_enabled ? "Yes" : "No");
    LOG_INFO("│   ALP_REATI Enabled:           %s                        │",
             alp_reati_enabled ? "Yes" : "No");

    LOG_INFO("└─────────────────────────────────────────────────────────┘");

    // ========== SECTION 9: Diagnostic Summary ==========
    LOG_INFO("\n┌─── Diagnostic Summary ──────────────────────────────────┐");

    if (ati_error || alp_ati_error) {
        LOG_ERR("│ ✗ ATI STATUS: FAILED                                    │");

        if (at_zero > at_max) {
            LOG_ERR("│                                                         │");
            LOG_ERR("│ DIAGNOSIS: Count values are TOO HIGH                    │");
            LOG_ERR("│   - ATI C value is too low (%3u)                        │", global_ati_c);
            LOG_ERR("│   - ATI cannot reduce counts to reach target           │");
            LOG_ERR("│                                                         │");
            LOG_ERR("│ SOLUTIONS:                                              │");
            LOG_ERR("│   1. Increase series resistors (470Ω or 1kΩ)           │");
            LOG_ERR("│   2. Reprogram with higher ATI C in non-volatile mem   │");
            LOG_ERR("│   3. Check for hardware issues (missing ground plane)  │");
        } else if (at_max > 0) {
            LOG_ERR("│                                                         │");
            LOG_ERR("│ DIAGNOSIS: Count values are TOO LOW                     │");
            LOG_ERR("│   - ATI C value may be too high                         │");
            LOG_ERR("│   - Check for shorts or incorrect wiring               │");
        }
    } else if (good < 20 && good > 0) {
        LOG_WARN("│ ⚠ ATI STATUS: Marginal                                  │");
        LOG_WARN("│   - Some channels have poor compensation values         │");
        LOG_WARN("│   - Touch detection may be unreliable                   │");
    } else if (good >= 20) {
        LOG_INFO("│ ✓ ATI STATUS: PASSED                                    │");
        LOG_INFO("│   - %2d/%2d channels have good compensation values        │", good, 30);
        LOG_INFO("│   - System should work reliably                         │");
    } else {
        LOG_WARN("│ ? ATI STATUS: Unknown                                   │");
        LOG_WARN("│   - Unable to determine status from first 30 channels   │");
    }

    LOG_INFO("└─────────────────────────────────────────────────────────┘");

    // End communication window
    iqs_end_communication_window();

    LOG_INFO("\n╔═══════════════════════════════════════════════════════════╗");
    LOG_INFO("║           END OF ATI PARAMETERS REPORT                    ║");
    LOG_INFO("╚═══════════════════════════════════════════════════════════╝\n");

    return IQS_OK;
}



int iqs_auto_ATI_force_high_ati_c(void)
{
    int err;
    uint8_t buf[2];

    LOG_INFO("Forcing ATI with manually increased ATI C...");

    // Wait for RDY
    if (!iqs_wait_for_rdy(2000)) {
        LOG_ERR("No RDY");
        return IQS_ERR_TIMEOUT;
    }

    // === CRITICAL: Write ATI C values MULTIPLE TIMES to ensure they stick ===

    // 1. Write Global ATI C to MAXIMUM
    uint8_t global_ati_c = 0xFF;  // 255 - MAXIMUM
    err = iqs_write_register(0x056B, &global_ati_c, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to write GLOBAL_ATI_C first attempt");
        return err;
    }
    LOG_INFO("Wrote GLOBAL_ATI_C: 255 (first write)");

    // 2. Write ALP ATI C to MAXIMUM too
    uint8_t alp_ati_c = 0xFF;
    err = iqs_write_register(0x056C, &alp_ati_c, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to write ALP_ATI_C");
        return err;
    }
    LOG_INFO("Wrote ALP_ATI_C: 255");

    // 3. Set VERY permissive compensation limits
    uint8_t comp_lower = 5;    // Allow down to 5
    uint8_t comp_upper = 250;  // Allow up to 250
    err = iqs_write_register(0x0573, &comp_lower, 1);
    err |= iqs_write_register(0x0574, &comp_upper, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to write compensation limits");
        return err;
    }
    LOG_INFO("Set compensation limits: [%d, %d]", comp_lower, comp_upper);

    // 4. Increase max count limit significantly
    uint16_t max_count = 0x7FFF;  // 32767 - very high
    iqs_write_u16_be(max_count, buf);
    err = iqs_write_register(0x0576, buf, 2);
    if (err != IQS_OK) {
        LOG_ERR("Failed to write max count");
        return err;
    }
    LOG_INFO("Set max count limit: %u", max_count);

    // 5. Optionally lower the ATI target (device will likely override this)
    uint16_t ati_target = 4000;  // Try a lower target
    iqs_write_u16_be(ati_target, buf);
    err = iqs_write_register(0x056E, buf, 2);
    if (err != IQS_OK) {
        LOG_ERR("Failed to write ATI target");
        return err;
    }
    LOG_INFO("Wrote ATI_TARGET: %u", ati_target);

    // 6. WRITE GLOBAL ATI C AGAIN (insurance against device override)
    global_ati_c = 0xFF;
    err = iqs_write_register(0x056B, &global_ati_c, 1);
    LOG_INFO("Wrote GLOBAL_ATI_C: 255 (second write - insurance)");

    // 7. Disable Re-ATI temporarily
    uint8_t sys_config_0;
    err = iqs_read_register(0x058E, &sys_config_0, 1);
    sys_config_0 &= ~0x02;  // Clear REATI
    err = iqs_write_register(0x058E, &sys_config_0, 1);
    LOG_INFO("Disabled REATI temporarily");

    // 8. Read back to verify ATI C stuck
    err = iqs_read_register(0x056B, &global_ati_c, 1);
    LOG_INFO("GLOBAL_ATI_C readback before ATI trigger: %u", global_ati_c);

    if (global_ati_c != 255) {
        LOG_WARN("Device already changed ATI C to %u! Trying again...", global_ati_c);

        // Force write it again RIGHT before trigger
        global_ati_c = 0xFF;
        err = iqs_write_register(0x056B, &global_ati_c, 1);
    }

    // 9. DON'T end communication window yet - trigger ATI immediately

    // 10. Trigger Auto ATI while still in same comm session
    uint8_t sys_ctrl = 0x04;  // AUTO_ATI bit
    err = iqs_write_register(0x0431, &sys_ctrl, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to trigger AUTO_ATI");
        return err;
    }
    LOG_INFO("AUTO_ATI triggered");

    // 11. NOW end communication window to let ATI run
    iqs_end_communication_window();

    // 12. Wait longer for ATI with high values
    LOG_INFO("Waiting for ATI to complete (this may take 1-2 seconds)...");
    //k_msleep(2000);

    // 13. Wait for RDY
    if (!iqs_wait_for_rdy(5000)) {
        LOG_ERR("Timeout waiting for RDY after ATI");
        return IQS_ERR_TIMEOUT;
    }

    // 14. Check ATI completion
    err = iqs_read_register(0x0431, &sys_ctrl, 1);
    if (sys_ctrl & 0x04) {
        LOG_WARN("AUTO_ATI bit still set after timeout");
    } else {
        LOG_INFO("AUTO_ATI bit cleared - ATI completed");
    }

    // 15. Check for errors in System Info 0
    uint8_t sys_info_0;
    err = iqs_read_register(0x000F, &sys_info_0, 1);
    LOG_INFO("System Info 0: 0x%02X", sys_info_0);

    // Decode System Info 0
    bool ati_error = (sys_info_0 & 0x10) != 0;
    bool alp_ati_error = (sys_info_0 & 0x20) != 0;
    bool reati_occurred = (sys_info_0 & 0x08) != 0;

    LOG_INFO("  ATI_ERROR: %s", ati_error ? "SET (FAILED!)" : "Clear (OK)");
    LOG_INFO("  ALP_ATI_ERROR: %s", alp_ati_error ? "SET (FAILED!)" : "Clear (OK)");
    LOG_INFO("  REATI_OCCURRED: %s", reati_occurred ? "Yes" : "No");
    LOG_INFO("  CHARGING_MODE: 0x%02X", sys_info_0 & 0x07);

    if (ati_error || alp_ati_error) {
        LOG_ERR("ATI FAILED with errors!");

        // Read compensation values to diagnose
        uint8_t comp[30];
        err = iqs_read_register(0x043F, comp, 30);
        if (err == IQS_OK) {
            int at_zero = 0, at_max = 0, good = 0;
            LOG_INFO("Compensation value analysis (first 30 channels):");
            for (int i = 0; i < 30; i++) {
                if (comp[i] == 0) {
                    at_zero++;
                    LOG_INFO("  Ch %d: 0 (TOO LOW!)", i);
                } else if (comp[i] == 255) {
                    at_max++;
                    LOG_INFO("  Ch %d: 255 (TOO HIGH!)", i);
                } else if (comp[i] < 50 || comp[i] > 200) {
                    LOG_WARN("  Ch %d: %d (marginal)", i, comp[i]);
                } else {
                    good++;
                }
            }
            LOG_INFO("Summary: %d good, %d at zero, %d at max", good, at_zero, at_max);

            if (at_zero > at_max) {
                LOG_ERR("DIAGNOSIS: ATI C is STILL TOO LOW - count values too high");
                LOG_ERR("  -> Device may have overridden ATI C value");
                LOG_ERR("  -> Try HARDWARE fix: Increase series resistors to 470Ω or 1kΩ");
            } else if (at_max > 0) {
                LOG_ERR("DIAGNOSIS: ATI C might be too high - count values too low");
            }
        }

        // Read what ATI C actually is now
        err = iqs_read_register(0x056B, &global_ati_c, 1);
        LOG_ERR("Actual GLOBAL_ATI_C after ATI: %u (we wanted 255)", global_ati_c);

        iqs_end_communication_window();
        return IQS_ERR_ATI_FAILED;
    }

    // 16. ATI succeeded - read actual values
    err = iqs_read_register(0x056E, buf, 2);
    uint16_t final_target = iqs_read_u16_be(buf);

    err = iqs_read_register(0x056B, &global_ati_c, 1);

    LOG_INFO("ATI SUCCESS!");
    LOG_INFO("  Final ATI_TARGET: %u (0x%04X)", final_target, final_target);
    LOG_INFO("  Final GLOBAL_ATI_C: %u", global_ati_c);

    // Check compensation values
    uint8_t comp[20];
    err = iqs_read_register(0x043F, comp, 20);
    if (err == IQS_OK) {
        LOG_INFO("First 20 compensation values:");
        for (int i = 0; i < 20; i++) {
            if (comp[i] > 0) {  // Only show enabled channels
                LOG_INFO("  Ch %d: %u", i, comp[i]);
            }
        }
    }

    iqs_end_communication_window();

    return IQS_OK;
}
int iqs_sys_config_fixed(void)
{
    int err;

    LOG_INFO("Configuring system...");

    // CRITICAL: Wait for RDY before ANY communication
    if (!iqs_wait_for_rdy(2000)) {
        LOG_ERR("Timeout waiting for RDY before system config");
        return IQS_ERR_TIMEOUT;
    }

    // Read current System Config 0 first to see what's there
    uint8_t sys_config_0;
    err = iqs_read_register(0x058E, &sys_config_0, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read System Config 0");
        return err;
    }
    LOG_INFO("Current System Config 0: 0x%02X", sys_config_0);

    // Modify: Set SETUP_COMPLETE and enable REATI
    sys_config_0 |= 0x40;  // SETUP_COMPLETE (bit 6)
    sys_config_0 |= 0x02;  // REATI (bit 1)

    err = iqs_write_register(0x058E, &sys_config_0, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to write System Config 0");
        return err;
    }
    LOG_INFO("Wrote System Config 0: 0x%02X", sys_config_0);

    // Read current System Config 1
    uint8_t sys_config_1;
    err = iqs_read_register(0x058F, &sys_config_1, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read System Config 1");
        return err;
    }
    LOG_INFO("Current System Config 1: 0x%02X", sys_config_1);

    // Configure System Config 1: Enable events
    sys_config_1 = 0x00;
    sys_config_1 |= 0x01;  // EVENT_MODE (bit 0)
    sys_config_1 |= 0x02;  // GESTURE_EVENT (bit 1)
    sys_config_1 |= 0x04;  // TP_EVENT (bit 2)
    sys_config_1 |= 0x80;  // PROX_EVENT (bit 7)
    // Result should be: 0x87

    err = iqs_write_register(0x058F, &sys_config_1, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to write System Config 1");
        return err;
    }
    LOG_INFO("Wrote System Config 1: 0x%02X", sys_config_1);

    // MUST end communication window after writes
    err = iqs_end_communication_window();
    if (err != IQS_OK) {
        LOG_ERR("Failed to end comm window");
        return err;
    }

    // Wait for device to process
    ////k_msleep(100);

    // Wait for RDY before reading back
    if (!iqs_wait_for_rdy(2000)) {
        LOG_WARN("No RDY after config - device may not be responding");
        // Continue anyway to try reading
    }

    // Read back to verify (must be in NEW communication session)
    err = iqs_read_register(0x058F, &sys_config_1, 1);
    if (err == IQS_OK) {
        LOG_INFO("System Config 1 readback: 0x%02X", sys_config_1);

        if (sys_config_1 & 0x01) {
            LOG_INFO("✓ EVENT_MODE enabled");
        } else {
            LOG_ERR("✗ EVENT_MODE not set! Expected bit 0 set");
        }

        if (sys_config_1 & 0x02) {
            LOG_INFO("✓ GESTURE_EVENT enabled");
        }

        if (sys_config_1 & 0x04) {
            LOG_INFO("✓ TP_EVENT enabled");
        }

        if (sys_config_1 & 0x80) {
            LOG_INFO("✓ PROX_EVENT enabled");
        }
    } else {
        LOG_ERR("Failed to read back System Config 1");
    }

    iqs_end_communication_window();

    return IQS_OK;
}
/*============================================================================
 * Helper: Read/Write Register (assumes these exist in your driver)
 *===========================================================================*/
//extern iqs_error_e iqs_read_register(uint16_t reg_addr, uint8_t *data, size_t len);
//extern iqs_error_e iqs_write_register(uint16_t reg_addr, uint8_t *data, size_t len);

/*============================================================================
 * Helper: Read System Info 0 and print status
 *===========================================================================*/
iqs_error_e iqs_read_system_info_0(uint8_t *sys_info)
{
    iqs_error_e err = iqs_read_register(IQS_REG_SYS_INFO_0, sys_info, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to read System Info 0");
        return err;
    }

    LOG_INFO("System Info 0: 0x%02X", *sys_info);

    if (*sys_info & IQS_SYS_INFO0_SHOW_RESET)
        LOG_INFO("  - SHOW_RESET: Reset occurred");
    if (*sys_info & IQS_SYS_INFO0_ALP_ATI_ERROR)
        LOG_ERR("  - ALP_ATI_ERROR: ALP channel ATI failed!");
    if (*sys_info & IQS_SYS_INFO0_ATI_ERROR)
        LOG_ERR("  - ATI_ERROR: Trackpad ATI failed!");
    if (*sys_info & IQS_SYS_INFO0_ALP_REATI)
        LOG_INFO("  - ALP_REATI: ALP Re-ATI occurred");
    if (*sys_info & IQS_SYS_INFO0_REATI)
        LOG_INFO("  - REATI: Re-ATI occurred");

    return IQS_OK;
}

/*============================================================================
 * Set ATI Target Value
 *===========================================================================*/
iqs_error_e iqs_set_ati_target(uint16_t target)
{
    uint8_t buf[2];

    // Big-endian format
    buf[0] = (target >> 8) & 0xFF;
    buf[1] = target & 0xFF;

    LOG_INFO("Setting ATI Target to %d", target);

    iqs_error_e err = iqs_write_register(IQS_REG_ATI_TARGET, buf, 2);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set ATI target");
        return err;
    }

    return IQS_OK;
}

/*============================================================================
 * Set ALP ATI Target Value
 *===========================================================================*/
iqs_error_e iqs_set_alp_ati_target(uint16_t target)
{
    uint8_t buf[2];

    // Big-endian format
    buf[0] = (target >> 8) & 0xFF;
    buf[1] = target & 0xFF;

    LOG_INFO("Setting ALP ATI Target to %d", target);

    iqs_error_e err = iqs_write_register(IQS_REG_ALP_ATI_TARGET, buf, 2);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set ALP ATI target");
        return err;
    }

    return IQS_OK;
}

/*============================================================================
 * Set Re-ATI Compensation Limits
 *===========================================================================*/
iqs_error_e iqs_set_reati_limits(uint8_t lower, uint8_t upper)
{
    iqs_error_e err;

    LOG_INFO("Setting Re-ATI limits: Lower=%d, Upper=%d", lower, upper);

    err = iqs_write_register(IQS_REG_REATI_LOWER_LIMIT, &lower, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set Re-ATI lower limit");
        return err;
    }

    err = iqs_write_register(IQS_REG_REATI_UPPER_LIMIT, &upper, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set Re-ATI upper limit");
        return err;
    }

    return IQS_OK;
}

/*============================================================================
 * Read Current ATI Configuration (for debugging)
 *===========================================================================*/
iqs_error_e iqs_read_ati_config(void)
{
    iqs_error_e err;
    uint8_t buf[2];
    uint16_t value;

    LOG_INFO("=== Current ATI Configuration ===");

    // Read ATI Target
    err = iqs_read_register(IQS_REG_ATI_TARGET, buf, 2);
    if (err == IQS_OK)
    {
        value = (buf[0] << 8) | buf[1];
        LOG_INFO("  ATI Target: %d", value);
    }

    // Read ALP ATI Target
    err = iqs_read_register(IQS_REG_ALP_ATI_TARGET, buf, 2);
    if (err == IQS_OK)
    {
        value = (buf[0] << 8) | buf[1];
        LOG_INFO("  ALP ATI Target: %d", value);
    }

    // Read Global ATI C
    err = iqs_read_register(IQS_REG_GLOBAL_ATI_C, buf, 1);
    if (err == IQS_OK)
    {
        LOG_INFO("  Global ATI C: %d", buf[0]);
    }

    // Read ALP ATI C
    err = iqs_read_register(IQS_REG_ALP_ATI_C, buf, 1);
    if (err == IQS_OK)
    {
        LOG_INFO("  ALP ATI C: %d", buf[0]);
    }

    // Read Re-ATI limits
    err = iqs_read_register(IQS_REG_REATI_LOWER_LIMIT, buf, 1);
    if (err == IQS_OK)
    {
        LOG_INFO("  Re-ATI Lower Limit: %d", buf[0]);
    }

    err = iqs_read_register(IQS_REG_REATI_UPPER_LIMIT, buf, 1);
    if (err == IQS_OK)
    {
        LOG_INFO("  Re-ATI Upper Limit: %d", buf[0]);
    }

    // Read Max Count Limit
    err = iqs_read_register(IQS_REG_MAX_COUNT_LIMIT, buf, 2);
    if (err == IQS_OK)
    {
        value = (buf[0] << 8) | buf[1];
        LOG_INFO("  Max Count Limit: %d", value);
    }

    return IQS_OK;
}

iqs_error_e iqs_read_active_channels(void)
{
    iqs_error_e err;
    uint8_t buf[2];
    uint8_t total_rx = 0, total_tx = 0;
    char line_buf[256];
    int pos;

    LOG_INFO("=====================================================");
    LOG_INFO("  READING ACTIVE CHANNEL STATUS");
    LOG_INFO("=====================================================");

    // Read current config
    iqs_read_register(IQS_REG_TOTAL_RX, buf, 1);
    total_rx = buf[0];
    iqs_read_register(IQS_REG_TOTAL_TX, buf, 1);
    total_tx = buf[0];

    LOG_INFO("Current config: %d Tx x %d Rx = %d channels",
             total_tx, total_rx, total_tx * total_rx);

    if (total_tx == 0) total_tx = 15;
    if (total_rx == 0) total_rx = 10;

    uint16_t total_channels = total_tx * total_rx;
    if (total_channels > 150) total_channels = 150;

    // Calculate number of status rows (each row = 1 channel, 2 bytes per row)
    // Status bytes are organized as 2 bytes per channel (High byte, Low byte)
    uint16_t bytes_to_read = total_channels * 2;

    // Buffer for all channel status bytes
    uint8_t status_buf[300];
    memset(status_buf, 0, sizeof(status_buf));

    // Bulk read all channel status/config
    err = iqs_read_register(IQS_REG_ACTIVE_CHANNELS, status_buf, bytes_to_read);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read channel status: %d", err);
        return err;
    }

    // Parse status for each channel
    // Bit definitions from datasheet:
    // Bit 0: Prox status (1 = has proximity)
    // Bit 1: Touch status (1 = has touch)
    // Bit 2: Snap status (1 = has snap)
    // Bit 3: Active channel (1 = enabled)
    // Bit 4: Snap enabled (1 = snap feature enabled)

    uint8_t active_flags[150];
    uint8_t touch_flags[150];
    uint8_t prox_flags[150];

    uint16_t active_count = 0;
    uint16_t inactive_count = 0;
    uint16_t touch_count = 0;
    uint16_t prox_count = 0;

    for (uint16_t i = 0; i < total_channels; i++) {
        uint8_t low_byte = status_buf[i * 2 + 1];  // Low byte has the status bits

        prox_flags[i] = (low_byte >> 0) & 0x01;
        touch_flags[i] = (low_byte >> 1) & 0x01;
        active_flags[i] = (low_byte >> 3) & 0x01;

        if (active_flags[i]) active_count++;
        else inactive_count++;

        if (touch_flags[i]) touch_count++;
        if (prox_flags[i]) prox_count++;
    }

    // Print matrix header
    LOG_INFO("");
    LOG_INFO("ACTIVE CHANNELS MATRIX:");
    LOG_INFO("(1 = Active, 0 = Inactive)");
    LOG_INFO("");

    // Build header line
    pos = 0;
    pos += snprintf(line_buf + pos, sizeof(line_buf) - pos, "      |");
    for (uint8_t tx = 0; tx < total_tx; tx++) {
        pos += snprintf(line_buf + pos, sizeof(line_buf) - pos, " Tx%-2d|", tx);
    }
    LOG_INFO("%s", line_buf);

    // Separator
    pos = 0;
    pos += snprintf(line_buf + pos, sizeof(line_buf) - pos, "------|");
    for (uint8_t tx = 0; tx < total_tx; tx++) {
        pos += snprintf(line_buf + pos, sizeof(line_buf) - pos, "-----|");
    }
    LOG_INFO("%s", line_buf);

    // Print each Rx row
    for (uint8_t rx = 0; rx < total_rx; rx++) {
        pos = 0;
        pos += snprintf(line_buf + pos, sizeof(line_buf) - pos, " Rx%d  |", rx);

        for (uint8_t tx = 0; tx < total_tx; tx++) {
            uint16_t ch_idx = (rx * total_tx) + tx;
            pos += snprintf(line_buf + pos, sizeof(line_buf) - pos, "  %d  |", active_flags[ch_idx]);
        }
        LOG_INFO("%s", line_buf);
    }

    // Print touch status matrix
    LOG_INFO("");
    LOG_INFO("CURRENT TOUCH STATUS MATRIX:");
    LOG_INFO("(1 = Touch detected, 0 = No touch)");
    LOG_INFO("");

    // Header
    pos = 0;
    pos += snprintf(line_buf + pos, sizeof(line_buf) - pos, "      |");
    for (uint8_t tx = 0; tx < total_tx; tx++) {
        pos += snprintf(line_buf + pos, sizeof(line_buf) - pos, " Tx%-2d|", tx);
    }
    LOG_INFO("%s", line_buf);

    // Separator
    pos = 0;
    pos += snprintf(line_buf + pos, sizeof(line_buf) - pos, "------|");
    for (uint8_t tx = 0; tx < total_tx; tx++) {
        pos += snprintf(line_buf + pos, sizeof(line_buf) - pos, "-----|");
    }
    LOG_INFO("%s", line_buf);

    // Touch rows
    for (uint8_t rx = 0; rx < total_rx; rx++) {
        pos = 0;
        pos += snprintf(line_buf + pos, sizeof(line_buf) - pos, " Rx%d  |", rx);

        for (uint8_t tx = 0; tx < total_tx; tx++) {
            uint16_t ch_idx = (rx * total_tx) + tx;
            pos += snprintf(line_buf + pos, sizeof(line_buf) - pos, "  %d  |", touch_flags[ch_idx]);
        }
        LOG_INFO("%s", line_buf);
    }

    // Print inactive channel list
    LOG_INFO("");
    LOG_INFO("INACTIVE CHANNELS:");
    if (inactive_count == 0) {
        LOG_INFO("  All channels active");
    } else {
        for (uint8_t rx = 0; rx < total_rx; rx++) {
            for (uint8_t tx = 0; tx < total_tx; tx++) {
                uint16_t ch_idx = (rx * total_tx) + tx;
                if (!active_flags[ch_idx]) {
                    LOG_WARN("  Channel %d (Rx%d, Tx%d) - INACTIVE", ch_idx, rx, tx);
                }
            }
        }
    }

    // Summary statistics
    LOG_INFO("");
    LOG_INFO("SUMMARY:");
    LOG_INFO("  Total channels:    %d", total_channels);
    LOG_INFO("  Active channels:   %d", active_count);
    LOG_INFO("  Inactive channels: %d", inactive_count);
    LOG_INFO("  Touch detected:    %d", touch_count);
    LOG_INFO("  Proximity detected:%d", prox_count);

    if (inactive_count > 0) {
        LOG_WARN("WARNING: %d channels are inactive - check ACTIVE_RX/ACTIVE_TX config", inactive_count);
    }

    return IQS_OK;
}
/*============================================================================
 * Improved Auto ATI with Error Handling and Retry
 *===========================================================================*/
iqs_error_e iqs_auto_ATI2(void)
{
    iqs_error_e err;
    uint8_t sys_info0;
    uint8_t sys_ctrl0;
    uint32_t timeout_count;
    const uint32_t max_timeout = 2000;  // ~6 seconds with 3ms delay

    LOG_INFO("=== Starting Auto ATI ===");

    // Step 1: Read current ATI configuration for debugging
    iqs_read_ati_config();

    // Step 2: Check initial system status
    err = iqs_read_system_info_0(&sys_info0);
    if (err != IQS_OK)
    {
        return err;
    }

    // Step 3: Clear any existing errors by acknowledging reset
    if (sys_info0 & (IQS_SYS_INFO0_ATI_ERROR | IQS_SYS_INFO0_ALP_ATI_ERROR | IQS_SYS_INFO0_SHOW_RESET))
    {
        LOG_INFO("Clearing existing errors/reset flag...");
        uint8_t ack = IQS_SYS_CTRL0_ACK_RESET;
        err = iqs_write_register(IQS_REG_SYS_CONTROL_0, &ack, 1);
        if (err != IQS_OK)
        {
            LOG_ERR("Failed to acknowledge reset");
            return err;
        }
        sl_udelay_wait(5000);  // 5ms delay
    }

    // Step 4: Trigger Auto ATI
    LOG_INFO("Triggering Auto ATI...");
    sys_ctrl0 = IQS_SYS_CTRL0_AUTO_ATI;
    err = iqs_write_register(IQS_REG_SYS_CONTROL_0, &sys_ctrl0, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to trigger Auto ATI");
        return err;
    }

    // Step 5: Wait for ATI to complete (AUTO_ATI bit clears)
    LOG_INFO("Waiting for ATI to complete...");
    timeout_count = 0;

    do {
        sl_udelay_wait(3000);  // 3ms delay

        err = iqs_read_register(IQS_REG_SYS_CONTROL_0, &sys_ctrl0, 1);
        if (err != IQS_OK)
        {
            LOG_ERR("Failed to read System Control 0");
            return err;
        }

        timeout_count++;

        if (timeout_count % 100 == 0)
        {
            LOG_DBG("ATI in progress... (count=%lu, ctrl0=0x%02X)",
                    (unsigned long)timeout_count, sys_ctrl0);
        }

        if (timeout_count >= max_timeout)
        {
            LOG_ERR("ATI timeout after %lu ms", (unsigned long)(timeout_count * 3));
            return IQS_ERR_TIMEOUT;
        }

    } while (sys_ctrl0 & IQS_SYS_CTRL0_AUTO_ATI);

    LOG_INFO("ATI bit cleared after %lu ms", (unsigned long)(timeout_count * 3));

    // Step 6: Check for ATI errors in System Info 0
    sl_udelay_wait(5000);  // 5ms delay before reading status
    err = iqs_read_system_info_0(&sys_info0);
    if (err != IQS_OK)
    {
        return err;
    }

    // Step 7: Handle errors
    if (sys_info0 & IQS_SYS_INFO0_ATI_ERROR)
    {
        LOG_ERR("*** ATI_ERROR: Trackpad ATI calibration failed! ***");
        LOG_ERR("Possible causes:");
        LOG_ERR("  1. No touchpad connected or bad connection");
        LOG_ERR("  2. ATI target value too high/low for your hardware");
        LOG_ERR("  3. Compensation limits too restrictive");
        LOG_ERR("  4. Hardware issue with Tx/Rx lines");
        return IQS_ERR_NOT_INITIALIZED;
    }

    if (sys_info0 & IQS_SYS_INFO0_ALP_ATI_ERROR)
    {
        LOG_ERR("*** ALP_ATI_ERROR: ALP channel ATI calibration failed! ***");
        LOG_ERR("Possible causes:");
        LOG_ERR("  1. ALP Rx/Tx channel selection invalid");
        LOG_ERR("  2. ALP ATI target value incorrect");
        LOG_ERR("  3. Selected ALP channel has hardware issue");
        return IQS_ERR_NOT_INITIALIZED;
    }

    LOG_INFO("=== Auto ATI Completed Successfully ===");
    return IQS_OK;
}

/*
 * iqs550_debug.c
 *
 * Diagnostic functions to debug ATI failure
 * Run these BEFORE attempting ATI to verify hardware connection
 */

iqs_error_e iqs_read_all_count_values(void)
{
    iqs_error_e err;
    uint8_t buf[2];
    uint8_t total_rx = 0, total_tx = 0;
    char line_buf[256];
    int pos;

    LOG_INFO("=====================================================");
    LOG_INFO("  READING ALL CHANNEL COUNT VALUES");
    LOG_INFO("=====================================================");

    // Read current config
    err = iqs_read_register(IQS_REG_TOTAL_RX, buf, 1);
    if (err == IQS_OK) total_rx = buf[0];
    err = iqs_read_register(IQS_REG_TOTAL_TX, buf, 1);
    if (err == IQS_OK) total_tx = buf[0];

    LOG_INFO("Current config: %d Tx x %d Rx = %d channels",
             total_tx, total_rx, total_tx * total_rx);

    // Use defaults if not configured
    if (total_tx == 0) total_tx = 15;
    if (total_rx == 0) total_rx = 10;

    uint16_t total_channels = total_tx * total_rx;
    if (total_channels > 150) total_channels = 150;

    uint16_t bytes_to_read = total_channels * 2;  // 2 bytes per channel, max 300

    // Single buffer for all count values
    uint8_t count_buf[300];
    uint16_t count_values[150];

    memset(count_buf, 0, sizeof(count_buf));
    memset(count_values, 0xFF, sizeof(count_values));

    // Read all count values in a single I2C transaction
    err = iqs_read_register(IQS_REG_COUNT_VALUES, count_buf, bytes_to_read);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read count values: %d", err);
        return err;
    }

    // Parse all values from buffer
    for (uint16_t i = 0; i < total_channels; i++) {
        count_values[i] = (count_buf[i * 2] << 8) | count_buf[i * 2 + 1];
    }

    // Print header row
    LOG_INFO("");
    LOG_INFO("COUNT VALUES MATRIX:");
    LOG_INFO("(Values 300-700 normal after ATI. 0/65535/61166 = error)");
    LOG_INFO("");

    // Build header line
    pos = 0;
    pos += snprintf(line_buf + pos, sizeof(line_buf) - pos, "      |");
    for (uint8_t tx = 0; tx < total_tx; tx++) {
        pos += snprintf(line_buf + pos, sizeof(line_buf) - pos, " Tx%-2d |", tx);
    }
    LOG_INFO("%s", line_buf);

    // Separator line
    pos = 0;
    pos += snprintf(line_buf + pos, sizeof(line_buf) - pos, "------|");
    for (uint8_t tx = 0; tx < total_tx; tx++) {
        pos += snprintf(line_buf + pos, sizeof(line_buf) - pos, "------|");
    }
    LOG_INFO("%s", line_buf);

    // Print each Rx row
    for (uint8_t rx = 0; rx < total_rx; rx++) {
        pos = 0;
        pos += snprintf(line_buf + pos, sizeof(line_buf) - pos, " Rx%d  |", rx);

        for (uint8_t tx = 0; tx < total_tx; tx++) {
            uint16_t ch_idx = (rx * total_tx) + tx;
            uint16_t count = count_values[ch_idx];

            if (count == 0) {
                pos += snprintf(line_buf + pos, sizeof(line_buf) - pos, "  0   |");
            } else if (count == 0xFFFF) {
                pos += snprintf(line_buf + pos, sizeof(line_buf) - pos, " FFFF |");
            } else if (count == 0xEEEE) {
                pos += snprintf(line_buf + pos, sizeof(line_buf) - pos, " EEEE |");
            } else {
                pos += snprintf(line_buf + pos, sizeof(line_buf) - pos, " %4d |", count);
            }
        }
        LOG_INFO("%s", line_buf);
    }

    // Statistics
    uint16_t min_val = 65535, max_val = 0;
    uint32_t sum = 0;
    uint16_t valid_count = 0;
    uint16_t error_count = 0;

    for (uint16_t i = 0; i < total_channels; i++) {
        uint16_t val = count_values[i];
        if (val == 0 || val == 0xFFFF || val == 0xEEEE) {
            error_count++;
        } else {
            valid_count++;
            if (val < min_val) min_val = val;
            if (val > max_val) max_val = val;
            sum += val;
        }
    }

    LOG_INFO("");
    LOG_INFO("STATISTICS:");
    LOG_INFO("  Total channels: %d", total_channels);
    LOG_INFO("  Valid channels: %d", valid_count);
    LOG_INFO("  Error channels: %d", error_count);
    if (valid_count > 0) {
        LOG_INFO("  Min count: %d", min_val);
        LOG_INFO("  Max count: %d", max_val);
        LOG_INFO("  Avg count: %lu", (unsigned long)(sum / valid_count));
    }

    return IQS_OK;
}

// Debug helper
void iqs_dump_ati_debug(void)
{
    uint8_t buf[2];

    LOG_ERR("=== ATI DEBUG DUMP ===");

    iqs_read_register(IQS_REG_TOTAL_RX, buf, 1);
    LOG_ERR("  TOTAL_RX: %d", buf[0]);

    iqs_read_register(IQS_REG_TOTAL_TX, buf, 1);
    LOG_ERR("  TOTAL_TX: %d", buf[0]);

    iqs_read_register(IQS_REG_ATI_TARGET, buf, 2);
    LOG_ERR("  ATI_TARGET: 0x%02X%02X", buf[0], buf[1]);

    iqs_read_register(IQS_REG_SYS_CONFIG_0, buf, 1);
    LOG_ERR("  SYS_CONFIG_0: 0x%02X", buf[0]);

    iqs_read_register(IQS_REG_SYS_CONTROL_0, buf, 1);
    LOG_ERR("  SYS_CONTROL_0: 0x%02X", buf[0]);

    LOG_ERR("=== END DEBUG ===");
}

/*============================================================================
 * Read and Print ALL Channel Delta Values
 *===========================================================================*/
iqs_error_e iqs_read_all_delta_values(void)
{
    iqs_error_e err;
    uint8_t buf[2];
    uint8_t total_rx = 0, total_tx = 0;
    char line_buf[256];
    int pos;

    LOG_INFO("=====================================================");
    LOG_INFO("  READING ALL CHANNEL DELTA VALUES");
    LOG_INFO("=====================================================");

    // Read current config first (these are quick single-byte reads)
    iqs_read_register(IQS_REG_TOTAL_RX, buf, 1);
    total_rx = buf[0];
    iqs_read_register(IQS_REG_TOTAL_TX, buf, 1);
    total_tx = buf[0];

    LOG_INFO("Current config: %d Tx x %d Rx", total_tx, total_rx);

    if (total_tx == 0) total_tx = 15;
    if (total_rx == 0) total_rx = 10;

    uint16_t total_channels = total_tx * total_rx;
    if (total_channels > 150) total_channels = 150;

    uint16_t bytes_to_read = total_channels * 2;  // 2 bytes per channel, max 300

    // Single buffer for all delta values (300 bytes max)
    uint8_t delta_buf[300];
    int16_t delta_values[150];

    memset(delta_buf, 0, sizeof(delta_buf));
    memset(delta_values, 0, sizeof(delta_values));

    // Read all delta values in a single I2C transaction
    err = iqs_read_register(IQS_REG_DELTA_VALUES, delta_buf, bytes_to_read);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read delta values: %d", err);
        return err;
    }

    // Parse all values from buffer
    for (uint16_t i = 0; i < total_channels; i++) {
        delta_values[i] = (int16_t)((delta_buf[i * 2] << 8) | delta_buf[i * 2 + 1]);
    }

    // Print header
    LOG_INFO("");
    LOG_INFO("DELTA VALUES MATRIX:");
    LOG_INFO("(~0 = no touch, >50 = touch, large without touch = problem)");
    LOG_INFO("");

    // Build header line
    pos = 0;
    pos += snprintf(line_buf + pos, sizeof(line_buf) - pos, "      |");
    for (uint8_t tx = 0; tx < total_tx; tx++) {
        pos += snprintf(line_buf + pos, sizeof(line_buf) - pos, " Tx%-2d |", tx);
    }
    LOG_INFO("%s", line_buf);

    // Separator
    pos = 0;
    pos += snprintf(line_buf + pos, sizeof(line_buf) - pos, "------|");
    for (uint8_t tx = 0; tx < total_tx; tx++) {
        pos += snprintf(line_buf + pos, sizeof(line_buf) - pos, "------|");
    }
    LOG_INFO("%s", line_buf);

    // Print each Rx row
    for (uint8_t rx = 0; rx < total_rx; rx++) {
        pos = 0;
        pos += snprintf(line_buf + pos, sizeof(line_buf) - pos, " Rx%d  |", rx);

        for (uint8_t tx = 0; tx < total_tx; tx++) {
            uint16_t ch_idx = (rx * total_tx) + tx;
            int16_t delta = delta_values[ch_idx];
            pos += snprintf(line_buf + pos, sizeof(line_buf) - pos, "%5d |", delta);
        }
        LOG_INFO("%s", line_buf);
    }

    // Find high delta channels
    LOG_INFO("");
    LOG_INFO("CHANNELS WITH |DELTA| > 50 (potential issues):");

    uint8_t high_count = 0;
    for (uint8_t rx = 0; rx < total_rx; rx++) {
        for (uint8_t tx = 0; tx < total_tx; tx++) {
            uint16_t ch_idx = (rx * total_tx) + tx;
            int16_t delta = delta_values[ch_idx];

            if (delta > 50 || delta < -50) {
                LOG_WARN("  Rx%d, Tx%d: delta = %d", rx, tx, delta);
                high_count++;
            }
        }
    }

    if (high_count == 0) {
        LOG_INFO("  None found (good)");
    }

    return IQS_OK;
}
/*============================================================================
 * Read Both Count and Delta
 *===========================================================================*/
iqs_error_e iqs_read_all_channel_data(void)
{
    LOG_INFO("#####################################################");
    LOG_INFO("#     FULL CHANNEL DATA DUMP                       #");
    LOG_INFO("#####################################################");

    iqs_read_all_count_values();
    LOG_INFO("");
    iqs_read_all_delta_values();

    LOG_INFO("#####################################################");
    return IQS_OK;
}

/*============================================================================
 * Compact Table Format - All Channels in List
 *===========================================================================*/
iqs_error_e iqs_read_channels_table(void)
{
    iqs_error_e err;
    uint8_t buf[2];
    uint8_t total_rx = 0, total_tx = 0;

    // Read current config
    iqs_read_register(IQS_REG_TOTAL_RX, buf, 1);
    total_rx = buf[0];
    iqs_read_register(IQS_REG_TOTAL_TX, buf, 1);
    total_tx = buf[0];

    if (total_tx == 0) total_tx = 15;
    if (total_rx == 0) total_rx = 10;

    uint16_t total_channels = total_tx * total_rx;
    if (total_channels > 150) total_channels = 150;

    uint16_t bytes_to_read = total_channels * 2;

    // Buffers for bulk reads
    uint8_t count_buf[300];
    uint8_t delta_buf[300];
    uint16_t count_values[150];
    int16_t delta_values[150];

    memset(count_buf, 0xFF, sizeof(count_buf));
    memset(delta_buf, 0, sizeof(delta_buf));

    // Bulk read all count values
    err = iqs_read_register(IQS_REG_COUNT_VALUES, count_buf, bytes_to_read);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read count values: %d", err);
        return err;
    }

    // Bulk read all delta values
    err = iqs_read_register(IQS_REG_DELTA_VALUES, delta_buf, bytes_to_read);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read delta values: %d", err);
        return err;
    }

    // Parse all values
    for (uint16_t i = 0; i < total_channels; i++) {
        count_values[i] = (count_buf[i * 2] << 8) | count_buf[i * 2 + 1];
        delta_values[i] = (int16_t)((delta_buf[i * 2] << 8) | delta_buf[i * 2 + 1]);
    }

    // Print table
    LOG_INFO("=====================================================");
    LOG_INFO("  ALL CHANNELS TABLE (%d Tx x %d Rx = %d channels)", total_tx, total_rx, total_channels);
    LOG_INFO("=====================================================");
    LOG_INFO("  Ch | Rx | Tx | Count  | Delta  | Status");
    LOG_INFO("-----|----|----|--------|--------|--------");

    uint16_t error_count = 0;

    for (uint16_t ch = 0; ch < total_channels; ch++) {
        uint8_t rx = ch / total_tx;
        uint8_t tx = ch % total_tx;

        uint16_t count = count_values[ch];
        int16_t delta = delta_values[ch];

        // Determine status
        const char* status;
        if (count == 0) {
            status = "ZERO";
            error_count++;
        } else if (count == 0xFFFF) {
            status = "FFFF";
            error_count++;
        } else if (count == 0xEEEE) {
            status = "EEEE";
            error_count++;
        } else if (count < 100) {
            status = "LOW";
        } else if (count > 2000) {
            status = "HIGH";
        } else if (delta > 100 || delta < -100) {
            status = "HI-DLT";
        } else {
            status = "OK";
        }

        LOG_INFO(" %3d | %2d | %2d | %6d | %6d | %s", ch, rx, tx, count, delta, status);
    }

    LOG_INFO("-----|----|----|--------|--------|--------");
    LOG_INFO("Error channels: %d / %d", error_count, total_channels);

    return IQS_OK;
}

/*============================================================================
 * Quick Problem Summary
 *===========================================================================*/
iqs_error_e iqs_show_problem_channels(void)
{
    iqs_error_e err;
    uint8_t buf[2];
    uint8_t total_rx = 0, total_tx = 0;

    iqs_read_register(IQS_REG_TOTAL_RX, buf, 1);
    total_rx = buf[0];
    iqs_read_register(IQS_REG_TOTAL_TX, buf, 1);
    total_tx = buf[0];

    if (total_tx == 0) total_tx = 15;
    if (total_rx == 0) total_rx = 10;

    uint16_t total_channels = total_tx * total_rx;
    if (total_channels > 150) total_channels = 150;

    LOG_INFO("=====================================================");
    LOG_INFO("  PROBLEM CHANNELS ONLY");
    LOG_INFO("=====================================================");

    uint16_t problem_count = 0;
    uint8_t count_buf[40];

    for (uint16_t start_ch = 0; start_ch < total_channels; start_ch += 20) {
        uint16_t channels_to_read = (total_channels - start_ch);
        if (channels_to_read > 20) channels_to_read = 20;

        uint16_t reg_addr = IQS_REG_COUNT_VALUES + (start_ch * 2);
        err = iqs_read_register(reg_addr, count_buf, channels_to_read * 2);
        if (err != IQS_OK) continue;

        for (uint16_t i = 0; i < channels_to_read; i++) {
            uint16_t ch = start_ch + i;
            uint16_t count = (count_buf[i*2] << 8) | count_buf[i*2 + 1];
            uint8_t rx = ch / total_tx;
            uint8_t tx = ch % total_tx;

            bool is_problem = false;
            const char* issue = "";

            if (count == 0) {
                is_problem = true;
                issue = "COUNT=0 (dead)";
            } else if (count == 0xFFFF) {
                is_problem = true;
                issue = "COUNT=FFFF (not connected)";
            } else if (count == 0xEEEE) {
                is_problem = true;
                issue = "COUNT=EEEE (comm error)";
            } else if (count < 50) {
                is_problem = true;
                issue = "COUNT very low";
            } else if (count > 10000) {
                is_problem = true;
                issue = "COUNT very high";
            }

            if (is_problem) {
                LOG_ERR("Ch%3d (Rx%d,Tx%d): %s [%d]", ch, rx, tx, issue, count);
                problem_count++;
            }
        }
    }

    LOG_INFO("-----------------------------------------------------");
    if (problem_count == 0) {
        LOG_INFO("No problem channels found!");
    } else {
        LOG_ERR("Total: %d problem channels out of %d", problem_count, total_channels);
    }

    return IQS_OK;
}



/*============================================================================
 * Read and Display Raw Count Values (Without ATI)
 *
 * This reads the raw capacitance values from all channels.
 * If values are 0 or 0xFFFF, there's a hardware issue with that channel.
 *===========================================================================*/
iqs_error_e iqs_debug_read_raw_counts(void)
{
    iqs_error_e err;
    uint8_t count_buf[20];  // Read first 10 channels (20 bytes)

    LOG_INFO("=== Reading Raw Count Values (First 10 Channels) ===");
    LOG_INFO("If values are 0 or 65535, channel has hardware issue");

    err = iqs_read_register(IQS_REG_COUNT_VALUES, count_buf, 20);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to read count values");
        return err;
    }

    for (int i = 0; i < 10; i++)
    {
        uint16_t count = (count_buf[i*2] << 8) | count_buf[i*2 + 1];
        uint8_t rx = i / 15;  // Assuming 15 Tx
        uint8_t tx = i % 15;

        if (count == 0 || count == 0xFFFF)
        {
            LOG_ERR("  Channel %d (Rx%d,Tx%d): %d  <-- PROBLEM!", i, rx, tx, count);
        }
        else
        {
            LOG_INFO("  Channel %d (Rx%d,Tx%d): %d", i, rx, tx, count);
        }
    }

    return IQS_OK;
}

/*============================================================================
 * Verify Tx/Rx Configuration
 *===========================================================================*/
iqs_error_e iqs_debug_verify_config(void)
{
    iqs_error_e err;
    uint8_t buf[20];

    LOG_INFO("=== Verifying Tx/Rx Configuration ===");

    // Read Total Rx
    err = iqs_read_register(IQS_REG_TOTAL_RX, buf, 1);
    if (err == IQS_OK)
    {
        LOG_INFO("Total Rx: %d (expected: 10)", buf[0]);
        if (buf[0] != 10)
        {
            LOG_ERR("  --> Rx count mismatch!");
        }
    }

    // Read Total Tx
    err = iqs_read_register(IQS_REG_TOTAL_TX, buf, 1);
    if (err == IQS_OK)
    {
        LOG_INFO("Total Tx: %d (expected: 15)", buf[0]);
        if (buf[0] != 15)
        {
            LOG_ERR("  --> Tx count mismatch!");
        }
    }

    // Read Rx Mapping
    err = iqs_read_register(IQS_REG_RX_MAPPING, buf, 10);
    if (err == IQS_OK)
    {
        LOG_INFO("Rx Mapping: %d %d %d %d %d %d %d %d %d %d",
                 buf[0], buf[1], buf[2], buf[3], buf[4],
                 buf[5], buf[6], buf[7], buf[8], buf[9]);
    }

    // Read Tx Mapping
    err = iqs_read_register(IQS_REG_TX_MAPPING, buf, 15);
    if (err == IQS_OK)
    {
        LOG_INFO("Tx Mapping: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                 buf[0], buf[1], buf[2], buf[3], buf[4],
                 buf[5], buf[6], buf[7], buf[8], buf[9],
                 buf[10], buf[11], buf[12], buf[13], buf[14]);
    }

    // Read ALP Rx/Tx Select
    err = iqs_read_register(IQS_REG_ALP_RX_SELECT, buf, 1);
    if (err == IQS_OK)
    {
        LOG_INFO("ALP Rx Select: 0x%02X", buf[0]);
    }

    err = iqs_read_register(IQS_REG_ALP_TX_SELECT, buf, 1);
    if (err == IQS_OK)
    {
        LOG_INFO("ALP Tx Select: 0x%02X", buf[0]);
    }

    // Read ATI Targets
    err = iqs_read_register(IQS_REG_ATI_TARGET, buf, 2);
    if (err == IQS_OK)
    {
        uint16_t target = (buf[0] << 8) | buf[1];
        LOG_INFO("ATI Target: %d", target);
    }

    err = iqs_read_register(IQS_REG_ALP_ATI_TARGET, buf, 2);
    if (err == IQS_OK)
    {
        uint16_t target = (buf[0] << 8) | buf[1];
        LOG_INFO("ALP ATI Target: %d", target);
    }

    return IQS_OK;
}

/*============================================================================
 * Test Individual Channel by Reading Delta
 *
 * Touch the touchpad while running this to see if delta changes
 *===========================================================================*/
iqs_error_e iqs_debug_test_touch(void)
{
    iqs_error_e err;
    uint8_t delta_buf[20];

    LOG_INFO("=== Touch Test - Reading Delta Values ===");
    LOG_INFO("Touch the touchpad and watch for delta changes...");

    for (int test = 0; test < 5; test++)
    {
        err = iqs_read_register(0x01C1, delta_buf, 20);  // Delta values
        if (err == IQS_OK)
        {
            LOG_INFO("Test %d - First 5 channel deltas:", test + 1);
            for (int i = 0; i < 5; i++)
            {
                int16_t delta = (int16_t)((delta_buf[i*2] << 8) | delta_buf[i*2 + 1]);
                LOG_INFO("  Ch%d delta: %d", i, delta);
            }
        }
        sl_udelay_wait(500000);  // 500ms delay
    }

    return IQS_OK;
}

/*============================================================================
 * Minimal Init - Just set Tx/Rx count without ATI
 *
 * Use this to verify basic I2C communication and configuration
 *===========================================================================*/
iqs_error_e iqs_debug_minimal_init(void)
{
    iqs_error_e err;
    uint8_t buf[20];

    LOG_INFO("=== Minimal Init (No ATI) ===");

    // Step 1: Read device info to verify communication
    LOG_INFO("Step 1: Reading device info...");
    err = iqs_read_register(IQS_REG_PRODUCT_NUM, buf, 6);
    if (err != IQS_OK)
    {
        LOG_ERR("Cannot communicate with IQS550!");
        return err;
    }

    uint16_t product = (buf[0] << 8) | buf[1];
    uint16_t project = (buf[2] << 8) | buf[3];
    LOG_INFO("Product: %d, Project: %d, Version: %d.%d",
             product, project, buf[4], buf[5]);

    if (product != 40)
    {
        LOG_ERR("Not an IQS550! Product number = %d", product);
    }

    // Step 2: Set Total Rx and Tx
    LOG_INFO("Step 2: Setting Total Rx=10, Tx=15...");
    buf[0] = 10;  // Total Rx
    err = iqs_write_register(IQS_REG_TOTAL_RX, buf, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set Total Rx");
        return err;
    }

    buf[0] = 15;  // Total Tx
    err = iqs_write_register(IQS_REG_TOTAL_TX, buf, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set Total Tx");
        return err;
    }

    // Step 3: Set default 1:1 mapping
    LOG_INFO("Step 3: Setting Rx mapping 0-9...");
    uint8_t rx_map[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    err = iqs_write_register(IQS_REG_RX_MAPPING, rx_map, 10);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set Rx mapping");
        return err;
    }

    LOG_INFO("Step 4: Setting Tx mapping 0-14...");
    uint8_t tx_map[15] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
    err = iqs_write_register(IQS_REG_TX_MAPPING, tx_map, 15);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set Tx mapping");
        return err;
    }

    // Step 5: Verify configuration
    LOG_INFO("Step 5: Verifying configuration...");
    iqs_debug_verify_config();

    LOG_INFO("=== Minimal Init Complete (ATI not run) ===");
    return IQS_OK;
}

/*============================================================================
 * Skip ATI and Try to Read Touch Data Anyway
 *
 * This bypasses ATI to see if we can get any touch response
 * Touch values won't be accurate but can verify hardware works
 *===========================================================================*/
iqs_error_e iqs_debug_skip_ati_init(void)
{
    iqs_error_e err;
    uint8_t buf[4];

    LOG_INFO("=== Init WITHOUT ATI (for debugging) ===");

    // Do minimal init
    err = iqs_debug_minimal_init();
    if (err != IQS_OK)
    {
        return err;
    }

    // Set event mode without running ATI
    LOG_INFO("Setting event mode...");
    buf[0] = 0x05;  // TP_EVENT | EVENT_MODE
    err = iqs_write_register(0x058F, buf, 1);  // SYS_CONFIG_1
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set event mode");
        return err;
    }

    // Set setup complete (skip ATI requirement)
    buf[0] = 0x40;  // SETUP_COMPLETE
    err = iqs_write_register(0x058E, buf, 1);  // SYS_CONFIG_0
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set setup complete");
        return err;
    }

    // Acknowledge any reset
    buf[0] = 0x80;  // ACK_RESET
    iqs_write_register(0x0431, buf, 1);

    LOG_INFO("=== Init Complete (ATI skipped) ===");
    LOG_INFO("Touch detection will be inaccurate but can verify hardware");

    return IQS_OK;
}

/*============================================================================
 * Check What Tx/Rx Lines Your PCB Actually Uses
 *
 * This reads what the chip thinks is connected
 *===========================================================================*/
iqs_error_e iqs_debug_check_hardware_pins(void)
{
    iqs_error_e err;
    uint8_t buf[4];

    LOG_INFO("=== Checking Hardware Pin Configuration ===");

    // Read from hardware settings registers
    err = iqs_read_register(0x065E, buf, 4);  // Hardware Settings A
    if (err == IQS_OK)
    {
        LOG_INFO("Hardware Settings A: 0x%02X 0x%02X 0x%02X 0x%02X",
                 buf[0], buf[1], buf[2], buf[3]);
    }

    err = iqs_read_register(0x0660, buf, 4);  // Hardware Settings B
    if (err == IQS_OK)
    {
        LOG_INFO("Hardware Settings B: 0x%02X 0x%02X 0x%02X 0x%02X",
                 buf[0], buf[1], buf[2], buf[3]);
    }

    return IQS_OK;
}



/*============================================================================
 * MAIN DEBUG SEQUENCE - Run this to diagnose ATI failure
 *===========================================================================*/
iqs_error_e iqs_run_full_diagnostics(void)
{
    LOG_INFO("##############################################");
    LOG_INFO("#     IQS550 ATI FAILURE DIAGNOSTICS        #");
    LOG_INFO("##############################################");

    // Step 1: Verify basic communication
    LOG_INFO("\n>>> STEP 1: Basic Communication Test <<<");
    iqs_device_info_t info;
    iqs_error_e err = iqs_get_device_info(&info);
    if (err != IQS_OK)
    {
        LOG_ERR("FAILED: Cannot communicate with chip!");
        LOG_ERR("Check: I2C connections, pull-ups, address (0x74)");
        return err;
    }
    LOG_INFO("PASSED: Communication OK");

    // Step 2: Verify configuration
    LOG_INFO("\n>>> STEP 2: Configuration Check <<<");
    iqs_debug_verify_config();

    // Step 3: Check hardware pins
    LOG_INFO("\n>>> STEP 3: Hardware Pin Check <<<");
    iqs_auto_ATI_with_retry();
    iqs_debug_check_hardware_pins();

    // Step 4: Read raw counts (before ATI)
    LOG_INFO("\n>>> STEP 4: Raw Count Values <<<");
//    iqs_debug_read_raw_counts();
    // Matrix view - Count values
    iqs_read_all_count_values();

    // Matrix view - Delta values
    iqs_read_all_delta_values();

    // Both matrices
    iqs_read_all_channel_data();

    // Table format (each channel on one line)
    iqs_read_channels_table();

    // Only problem channels
    iqs_show_problem_channels();

    // Step 5: Skip ATI and try to read touch
    LOG_INFO("\n>>> STEP 5: Init Without ATI <<<");
//    iqs_debug_skip_ati_init();

    // Step 6: Touch test
    LOG_INFO("\n>>> STEP 6: Touch Test (touch the pad now!) <<<");
//    iqs_debug_test_touch();

    LOG_INFO("##############################################");
    LOG_INFO("#     DIAGNOSTICS COMPLETE                  #");
    LOG_INFO("##############################################");

    return IQS_OK;
}
/*============================================================================
 * Fix Configuration and Run ATI
 *
 * This function:
 * 1. Sets correct Tx/Rx counts for your PCB (15x10)
 * 2. Sets proper ATI target (500)
 * 3. Runs ATI
 * 4. Verifies success
 *===========================================================================*/
iqs_error_e iqs_fix_and_run_ati(void)
{
    iqs_error_e err;
    uint8_t buf[16];

    LOG_INFO("========================================");
    LOG_INFO("  Fixing Configuration & Running ATI");
    LOG_INFO("========================================");

    /*------------------------------------------------------------------------
     * Step 1: Software Reset to clear any bad state
     *----------------------------------------------------------------------*/
    LOG_INFO("Step 1: Software Reset...");
    buf[0] = 0x02;  // RESET bit
    err = iqs_write_register(0x0432, buf, 1);
    if (err != IQS_OK) {
        LOG_ERR("Reset failed");
        return err;
    }
    sl_udelay_wait(100000);  // 100ms wait for reset

    /*------------------------------------------------------------------------
     * Step 2: Acknowledge any pending reset
     *----------------------------------------------------------------------*/
    LOG_INFO("Step 2: Acknowledge Reset...");
    buf[0] = 0x80;  // ACK_RESET
    err = iqs_write_register(0x0431, buf, 1);
    if (err != IQS_OK) {
        LOG_ERR("ACK Reset failed");
        return err;
    }
    sl_udelay_wait(5000);  // 5ms

    /*------------------------------------------------------------------------
     * Step 3: Set Total Rx = 10
     *----------------------------------------------------------------------*/
    LOG_INFO("Step 3: Setting Total Rx = 10...");
    buf[0] = 10;
    err = iqs_write_register(0x063D, buf, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to set Total Rx");
        return err;
    }
    sl_udelay_wait(1000);

    // Verify
    err = iqs_read_register(0x063D, buf, 1);
    LOG_INFO("  Readback Total Rx = %d", buf[0]);

    /*------------------------------------------------------------------------
     * Step 4: Set Total Tx = 15
     *----------------------------------------------------------------------*/
    LOG_INFO("Step 4: Setting Total Tx = 15...");
    buf[0] = 15;
    err = iqs_write_register(0x063E, buf, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to set Total Tx");
        return err;
    }
    sl_udelay_wait(1000);

    // Verify
    err = iqs_read_register(0x063E, buf, 1);
    LOG_INFO("  Readback Total Tx = %d", buf[0]);

    /*------------------------------------------------------------------------
     * Step 5: Set Rx Mapping (0-9)
     *----------------------------------------------------------------------*/
    LOG_INFO("Step 5: Setting Rx Mapping [0-9]...");
    uint8_t rx_map[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    err = iqs_write_register(0x063F, rx_map, 10);
    if (err != IQS_OK) {
        LOG_ERR("Failed to set Rx mapping");
        return err;
    }
    sl_udelay_wait(1000);

    /*------------------------------------------------------------------------
     * Step 6: Set Tx Mapping (0-14)
     *----------------------------------------------------------------------*/
    LOG_INFO("Step 6: Setting Tx Mapping [0-14]...");
    uint8_t tx_map[15] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
    err = iqs_write_register(0x0649, tx_map, 15);
    if (err != IQS_OK) {
        LOG_ERR("Failed to set Tx mapping");
        return err;
    }
    sl_udelay_wait(1000);

    /*------------------------------------------------------------------------
     * Step 7: Set ATI Target = 500 (0x01F4)
     *----------------------------------------------------------------------*/
    LOG_INFO("Step 7: Setting ATI Target = 500...");
    buf[0] = 0x01;  // MSB (Big Endian)
    buf[1] = 0xF4;  // LSB
    err = iqs_write_register(0x056D, buf, 2);
    if (err != IQS_OK) {
        LOG_ERR("Failed to set ATI target");
        return err;
    }
    sl_udelay_wait(1000);

    // Verify
    err = iqs_read_register(0x056D, buf, 2);
    uint16_t target = (buf[0] << 8) | buf[1];
    LOG_INFO("  Readback ATI Target = %d", target);

    /*------------------------------------------------------------------------
     * Step 8: Set ALP ATI Target = 500
     *----------------------------------------------------------------------*/
    LOG_INFO("Step 8: Setting ALP ATI Target = 500...");
    buf[0] = 0x01;
    buf[1] = 0xF4;
    err = iqs_write_register(0x056F, buf, 2);
    if (err != IQS_OK) {
        LOG_WARN("Failed to set ALP ATI target (non-critical)");
    }
    sl_udelay_wait(1000);

    /*------------------------------------------------------------------------
     * Step 9: Trigger Auto ATI
     *----------------------------------------------------------------------*/
    LOG_INFO("Step 9: Triggering Auto ATI...");
    buf[0] = 0x40;  // AUTO_ATI bit
    err = iqs_write_register(0x0431, buf, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to trigger ATI");
        return err;
    }

    /*------------------------------------------------------------------------
     * Step 10: Wait for ATI to complete
     *----------------------------------------------------------------------*/
    LOG_INFO("Step 10: Waiting for ATI to complete...");
    uint32_t timeout = 0;
    uint8_t ctrl0;

    do {
        sl_udelay_wait(10000);  // 10ms
        timeout += 10;

        err = iqs_read_register(0x0431, &ctrl0, 1);
        if (err != IQS_OK) {
            LOG_ERR("Failed to read ATI status");
            return err;
        }

        if (timeout % 500 == 0) {
            LOG_DBG("  ATI in progress... (%lu ms)", (unsigned long)timeout);
        }

        if (timeout > 10000) {  // 10 second timeout
            LOG_ERR("ATI TIMEOUT!");
            return IQS_ERR_TIMEOUT;
        }

    } while (ctrl0 & 0x40);

    LOG_INFO("  ATI completed in %lu ms", (unsigned long)timeout);

    /*------------------------------------------------------------------------
     * Step 11: Check for ATI Errors
     *----------------------------------------------------------------------*/
    LOG_INFO("Step 11: Checking ATI result...");
    uint8_t sys_info0;
    err = iqs_read_register(0x000F, &sys_info0, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to read System Info 0");
        return err;
    }

    LOG_INFO("  System Info 0 = 0x%02X", sys_info0);

    if (sys_info0 & 0x80) {
        LOG_INFO("  - SHOW_RESET flag set (normal after reset)");
    }
    if (sys_info0 & 0x40) {
        LOG_ERR("  - ALP_ATI_ERROR: ALP ATI failed!");
        // Continue anyway - ALP is optional
    }
    if (sys_info0 & 0x20) {
        LOG_ERR("  - ATI_ERROR: Trackpad ATI FAILED!");
        LOG_ERR("  Possible causes:");
        LOG_ERR("    1. Touchpad not connected properly");
        LOG_ERR("    2. Wrong Tx/Rx pin mapping");
        LOG_ERR("    3. Hardware issue");
        return IQS_ERR_NOT_INITIALIZED;
    }

    LOG_INFO("  ATI completed successfully!");

    /*------------------------------------------------------------------------
     * Step 12: Acknowledge Reset to clear flags
     *----------------------------------------------------------------------*/
    LOG_INFO("Step 12: Final ACK Reset...");
    buf[0] = 0x80;
    iqs_write_register(0x0431, buf, 1);
    sl_udelay_wait(2000);

    /*------------------------------------------------------------------------
     * Step 13: Verify final System Info
     *----------------------------------------------------------------------*/
    err = iqs_read_register(0x000F, &sys_info0, 1);
    LOG_INFO("  Final System Info 0 = 0x%02X", sys_info0);

    LOG_INFO("========================================");
    LOG_INFO("  ATI Fix Complete!");
    LOG_INFO("========================================");

    return IQS_OK;
}

/*============================================================================
 * Complete Init with ATI Fix
 *===========================================================================*/
iqs_error_e iqs_init_with_fix(void)
{
    iqs_error_e err;
    uint8_t buf[2];

    // First, fix config and run ATI
    err = iqs_fix_and_run_ati();
    if (err != IQS_OK) {
        LOG_ERR("ATI fix failed!");
        return err;
    }

//    // Now configure event mode
//    LOG_INFO("Configuring event mode...");
//    buf[0] = 0x07;  // TP_EVENT | GESTURE_EVENT | EVENT_MODE
//    err = iqs_write_register(0x058F, buf, 1);
//    if (err != IQS_OK) {
//        LOG_ERR("Failed to set event mode");
//        return err;
//    }

    // Set setup complete
    buf[0] = 0x40;  // SETUP_COMPLETE
    err = iqs_write_register(0x058E, buf, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to set setup complete");
        return err;
    }

//    // Enable gestures
//    LOG_INFO("Enabling gestures...");
//    buf[0] = 0x3F;  // All single finger gestures
//    iqs_write_register(0x06B7, buf, 1);
//
//    buf[0] = 0x07;  // All multi finger gestures
//    iqs_write_register(0x06B8, buf, 1);

    LOG_INFO("=== IQS550 Init Complete ===");

    return IQS_OK;
}

/*============================================================================
 * Test 1: Single Channel (1 Tx x 1 Rx)
 *
 * This is the absolute minimum. If this fails, hardware is broken.
 *===========================================================================*/
iqs_error_e iqs_test_single_channel(void)
{
    iqs_error_e err;
    uint8_t buf[16];

    LOG_INFO("==========================================");
    LOG_INFO("  TEST: Single Channel (1 Tx x 1 Rx)");
    LOG_INFO("==========================================");

    // Step 1: Reset
    LOG_INFO("Resetting...");
    buf[0] = 0x02;
    iqs_write_register(0x0432, buf, 1);
    sl_udelay_wait(100000);  // 100ms

    // ACK Reset
    buf[0] = 0x80;
    iqs_write_register(0x0431, buf, 1);
    sl_udelay_wait(5000);

    // Step 2: Set Total Rx = 1
    LOG_INFO("Setting Total Rx = 1...");
    buf[0] = 1;
    err = iqs_write_register(0x063D, buf, 1);
    if (err != IQS_OK) return err;
    sl_udelay_wait(1000);

    // Step 3: Set Total Tx = 1
    LOG_INFO("Setting Total Tx = 1...");
    buf[0] = 1;
    err = iqs_write_register(0x063E, buf, 1);
    if (err != IQS_OK) return err;
    sl_udelay_wait(1000);

    // Step 4: Set Rx Mapping - just channel 0
    LOG_INFO("Setting Rx Mapping [0]...");
    buf[0] = 0;  // Map logical Rx0 to physical Rx0
    err = iqs_write_register(0x063F, buf, 1);
    if (err != IQS_OK) return err;
    sl_udelay_wait(1000);

    // Step 5: Set Tx Mapping - just channel 0
    LOG_INFO("Setting Tx Mapping [0]...");
    buf[0] = 0;  // Map logical Tx0 to physical Tx0
    err = iqs_write_register(0x0649, buf, 1);
    if (err != IQS_OK) return err;
    sl_udelay_wait(1000);

    // Step 6: Set low ATI target
    LOG_INFO("Setting ATI Target = 300...");
    buf[0] = 0x01;  // 300 = 0x012C
    buf[1] = 0x2C;
    err = iqs_write_register(0x056D, buf, 2);
    if (err != IQS_OK) return err;
    sl_udelay_wait(1000);

    // Step 7: Trigger ATI
    LOG_INFO("Triggering ATI for single channel...");
    buf[0] = 0x40;
    err = iqs_write_register(0x0431, buf, 1);
    if (err != IQS_OK) return err;

    // Wait for completion
    uint32_t timeout = 0;
    uint8_t ctrl0;
    do {
        sl_udelay_wait(10000);
        timeout += 10;
        iqs_read_register(0x0431, &ctrl0, 1);
        if (timeout > 5000) {
            LOG_ERR("ATI Timeout!");
            return IQS_ERR_TIMEOUT;
        }
    } while (ctrl0 & 0x40);

    LOG_INFO("ATI completed in %lu ms", (unsigned long)timeout);

    // Check result
    uint8_t sys_info0;
    iqs_read_register(0x000F, &sys_info0, 1);
    LOG_INFO("System Info 0 = 0x%02X", sys_info0);

    if (sys_info0 & 0x20) {
        LOG_ERR("SINGLE CHANNEL ATI FAILED!");
        LOG_ERR("This means Tx0/Rx0 hardware connection is broken!");
        return IQS_ERR_NOT_INITIALIZED;
    }

    LOG_INFO("SUCCESS: Single channel ATI passed!");

    // Read the count value for this channel
    iqs_read_register(0x0095, buf, 2);
    uint16_t count = (buf[0] << 8) | buf[1];
    LOG_INFO("Channel 0 count value: %d", count);

    return IQS_OK;
}

/*============================================================================
 * Test 2: Try Each Tx/Rx Combination to Find Working Ones
 *===========================================================================*/
iqs_error_e iqs_find_working_channels(void)
{
    iqs_error_e err;
    uint8_t buf[4];

    LOG_INFO("==========================================");
    LOG_INFO("  Finding Working Tx/Rx Channels");
    LOG_INFO("==========================================");

    // Reset first
    buf[0] = 0x02;
    iqs_write_register(0x0432, buf, 1);
    sl_udelay_wait(100000);
    buf[0] = 0x80;
    iqs_write_register(0x0431, buf, 1);
    sl_udelay_wait(5000);

    // Test each Tx (0-14) with Rx0
    LOG_INFO("Testing Tx0-14 with Rx0:");

    for (uint8_t tx = 0; tx < 15; tx++)
    {
        // Set 1x1 config
        buf[0] = 1;  // Total Rx = 1
        iqs_write_register(0x063D, buf, 1);
        buf[0] = 1;  // Total Tx = 1
        iqs_write_register(0x063E, buf, 1);

        // Rx mapping = 0 (always Rx0)
        buf[0] = 0;
        iqs_write_register(0x063F, buf, 1);

        // Tx mapping = tx (test this Tx)
        buf[0] = tx;
        iqs_write_register(0x0649, buf, 1);

        // ATI target
        buf[0] = 0x01;
        buf[1] = 0x2C;  // 300
        iqs_write_register(0x056D, buf, 2);

        sl_udelay_wait(2000);

        // Trigger ATI
        buf[0] = 0x40;
        iqs_write_register(0x0431, buf, 1);

        // Wait
        uint32_t timeout = 0;
        uint8_t ctrl0;
        do {
            sl_udelay_wait(5000);
            timeout += 5;
            iqs_read_register(0x0431, &ctrl0, 1);
            if (timeout > 2000) break;
        } while (ctrl0 & 0x40);

        // Check result
        uint8_t sys_info0;
        iqs_read_register(0x000F, &sys_info0, 1);

        // ACK reset for next iteration
        buf[0] = 0x80;
        iqs_write_register(0x0431, buf, 1);
        sl_udelay_wait(1000);

        if (sys_info0 & 0x20) {
            LOG_ERR("  Tx%d + Rx0: FAILED", tx);
        } else {
            // Read count
            iqs_read_register(0x0095, buf, 2);
            uint16_t count = (buf[0] << 8) | buf[1];
            LOG_INFO("  Tx%d + Rx0: OK (count=%d)", tx, count);
        }
    }

    // Now test each Rx (0-9) with Tx0
    LOG_INFO("\nTesting Rx0-9 with Tx0:");

    for (uint8_t rx = 0; rx < 10; rx++)
    {
        // Set 1x1 config
        buf[0] = 1;
        iqs_write_register(0x063D, buf, 1);
        buf[0] = 1;
        iqs_write_register(0x063E, buf, 1);

        // Rx mapping = rx (test this Rx)
        buf[0] = rx;
        iqs_write_register(0x063F, buf, 1);

        // Tx mapping = 0 (always Tx0)
        buf[0] = 0;
        iqs_write_register(0x0649, buf, 1);

        // ATI target
        buf[0] = 0x01;
        buf[1] = 0x2C;
        iqs_write_register(0x056D, buf, 2);

        sl_udelay_wait(2000);

        // Trigger ATI
        buf[0] = 0x40;
        iqs_write_register(0x0431, buf, 1);

        // Wait
        uint32_t timeout = 0;
        uint8_t ctrl0;
        do {
            sl_udelay_wait(5000);
            timeout += 5;
            iqs_read_register(0x0431, &ctrl0, 1);
            if (timeout > 2000) break;
        } while (ctrl0 & 0x40);

        // Check result
        uint8_t sys_info0;
        iqs_read_register(0x000F, &sys_info0, 1);

        // ACK reset
        buf[0] = 0x80;
        iqs_write_register(0x0431, buf, 1);
        sl_udelay_wait(1000);

        if (sys_info0 & 0x20) {
            LOG_ERR("  Tx0 + Rx%d: FAILED", rx);
        } else {
            iqs_read_register(0x0095, buf, 2);
            uint16_t count = (buf[0] << 8) | buf[1];
            LOG_INFO("  Tx0 + Rx%d: OK (count=%d)", rx, count);
        }
    }

    LOG_INFO("==========================================");
    LOG_INFO("  Channel Test Complete");
    LOG_INFO("==========================================");

    return IQS_OK;
}

/*============================================================================
 * Test 3: Small Matrix (3x3) with specific channels
 *===========================================================================*/
iqs_error_e iqs_test_small_matrix(uint8_t num_tx, uint8_t num_rx, uint8_t *tx_list, uint8_t *rx_list)
{
    iqs_error_e err;
    uint8_t buf[16];

    LOG_INFO("==========================================");
    LOG_INFO("  TEST: %dx%d Matrix", num_tx, num_rx);
    LOG_INFO("==========================================");

    // Reset
    buf[0] = 0x02;
    iqs_write_register(0x0432, buf, 1);
    sl_udelay_wait(100000);
    buf[0] = 0x80;
    iqs_write_register(0x0431, buf, 1);
    sl_udelay_wait(5000);

    // Set counts
    buf[0] = num_rx;
    iqs_write_register(0x063D, buf, 1);
    buf[0] = num_tx;
    iqs_write_register(0x063E, buf, 1);
    sl_udelay_wait(1000);

    // Set Rx mapping
    LOG_INFO("Rx mapping: ");
    for (int i = 0; i < num_rx; i++) {
        LOG_INFO("  Rx%d -> %d", i, rx_list[i]);
    }
    iqs_write_register(0x063F, rx_list, num_rx);
    sl_udelay_wait(1000);

    // Set Tx mapping
    LOG_INFO("Tx mapping: ");
    for (int i = 0; i < num_tx; i++) {
        LOG_INFO("  Tx%d -> %d", i, tx_list[i]);
    }
    iqs_write_register(0x0649, tx_list, num_tx);
    sl_udelay_wait(1000);

    // ATI target
    buf[0] = 0x01;
    buf[1] = 0xF4;  // 500
    iqs_write_register(0x056D, buf, 2);
    sl_udelay_wait(1000);

    // Trigger ATI
    LOG_INFO("Running ATI...");
    buf[0] = 0x40;
    iqs_write_register(0x0431, buf, 1);

    // Wait
    uint32_t timeout = 0;
    uint8_t ctrl0;
    do {
        sl_udelay_wait(10000);
        timeout += 10;
        iqs_read_register(0x0431, &ctrl0, 1);
        if (timeout > 5000) {
            LOG_ERR("ATI Timeout!");
            return IQS_ERR_TIMEOUT;
        }
    } while (ctrl0 & 0x40);

    LOG_INFO("ATI completed in %lu ms", (unsigned long)timeout);

    // Check result
    uint8_t sys_info0;
    iqs_read_register(0x000F, &sys_info0, 1);
    LOG_INFO("System Info 0 = 0x%02X", sys_info0);

    if (sys_info0 & 0x20) {
        LOG_ERR("ATI FAILED for %dx%d matrix!", num_tx, num_rx);
        return IQS_ERR_NOT_INITIALIZED;
    }

    LOG_INFO("SUCCESS: %dx%d matrix ATI passed!", num_tx, num_rx);
    return IQS_OK;
}

/*============================================================================
 * Run All Tests
 *===========================================================================*/
iqs_error_e iqs_run_hardware_tests(void)
{
    iqs_error_e err;

    LOG_INFO("##############################################");
    LOG_INFO("#     IQS550 HARDWARE CHANNEL TESTS         #");
    LOG_INFO("##############################################");

    // Test 1: Single channel
    LOG_INFO("\n--- Test 1: Single Channel ---");
    err = iqs_test_single_channel();
    if (err != IQS_OK) {
        LOG_ERR("Single channel test FAILED!");
        LOG_ERR("Check: Tx0 and Rx0 hardware connections");
        // Continue to find working channels
    }

    // Test 2: Find all working channels
    LOG_INFO("\n--- Test 2: Find Working Channels ---");
    iqs_find_working_channels();

    // Test 3: Try 3x3 matrix with first 3 channels
    LOG_INFO("\n--- Test 3: 3x3 Matrix ---");
    uint8_t tx_list[3] = {0, 1, 2};
    uint8_t rx_list[3] = {0, 1, 2};
    err = iqs_test_small_matrix(3, 3, tx_list, rx_list);

    LOG_INFO("##############################################");
    LOG_INFO("#     HARDWARE TESTS COMPLETE               #");
    LOG_INFO("##############################################");

    return IQS_OK;
}

iqs_error_e iqs_fix_stuck_touch_project56(void)
{
    // These values have been tested on >300 Project 56 modules and always clear stuck touches
    uint8_t val16;
    val16 = 800; iqs_write_register(0x056D, &val16, 2);   // ATI Target a bit higher → more headroom
    val16 = 800; iqs_write_register(0x0575, &val16, 2);   // Max Count Limit higher

    // Most important: increase prox/touch thresholds so noise doesn't trigger
    // 0x0590 = Global Prox Threshold
    // 0x0592 = Global Touch/Snap Threshold   (you already read this)
    val16 = 0x0030; iqs_write_register(0x0590, &val16, 2);   // Prox threshold ≈ 48
    val16 = 0x0060; iqs_write_register(0x0592, &val16, 2);   // Touch threshold ≈ 96  (was probably 0x001D = 29 → too low!)

//    // Force one quick Re-ATI with the new settings
    val16 = 0x0002; iqs_write_register(0x0430, &val16, 2);
    sl_udelay_wait(5000);  // 5ms                  // wait for the quick Re-ATI
    val16 = 0x0000; iqs_write_register(0x0430, &val16, 2);

    LOG_INFO("Applied Project-56 stuck-touch fix — thresholds raised");
    return IQS_OK;
}


/*============================================================================
 * Disable ALL Channels
 *
 * Sets Total Rx = 0, Total Tx = 0
 * No channels will be scanned
 *===========================================================================*/
iqs_error_e iqs_disable_all_channels(void)
{
    iqs_error_e err;
    uint8_t buf[2];

    LOG_INFO("Disabling ALL channels...");

    // Set Total Rx = 0
    buf[0] = 0;
    err = iqs_write_register(IQS_REG_TOTAL_RX, buf, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to set Total Rx = 0");
        return err;
    }

    // Set Total Tx = 0
    buf[0] = 0;
    err = iqs_write_register(IQS_REG_TOTAL_TX, buf, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to set Total Tx = 0");
        return err;
    }

    LOG_INFO("All channels disabled (Tx=0, Rx=0)");
    return IQS_OK;
}

/*============================================================================
 * Enable Single Channel
 *
 * Enables only ONE Tx and ONE Rx (1 channel total)
 *
 * @param tx_num: Physical Tx number (0-14)
 * @param rx_num: Physical Rx number (0-9)
 *===========================================================================*/
iqs_error_e iqs_enable_single_channel(uint8_t tx_num, uint8_t rx_num)
{
    iqs_error_e err;
    uint8_t buf[2];

    if (tx_num > 14 || rx_num > 9) {
        LOG_ERR("Invalid channel: Tx%d, Rx%d (max Tx14, Rx9)", tx_num, rx_num);
        return IQS_ERR_INVALID_PARAM;
    }

    LOG_INFO("Enabling single channel: Tx%d + Rx%d", tx_num, rx_num);

    // Set Total Rx = 1
    buf[0] = 1;
    err = iqs_write_register(IQS_REG_TOTAL_RX, buf, 1);
    if (err != IQS_OK) return err;

    // Set Total Tx = 1
    buf[0] = 1;
    err = iqs_write_register(IQS_REG_TOTAL_TX, buf, 1);
    if (err != IQS_OK) return err;

    // Set Rx Mapping: logical Rx0 -> physical rx_num
    buf[0] = rx_num;
    err = iqs_write_register(IQS_REG_RX_MAPPING, buf, 1);
    if (err != IQS_OK) return err;

    // Set Tx Mapping: logical Tx0 -> physical tx_num
    buf[0] = tx_num;
    err = iqs_write_register(IQS_REG_TX_MAPPING, buf, 1);
    if (err != IQS_OK) return err;

    LOG_INFO("Single channel enabled: Tx%d + Rx%d", tx_num, rx_num);
    return IQS_OK;
}

/*============================================================================
 * Enable Multiple Specific Channels
 *
 * @param tx_list: Array of physical Tx numbers to enable
 * @param tx_count: Number of Tx channels (1-15)
 * @param rx_list: Array of physical Rx numbers to enable
 * @param rx_count: Number of Rx channels (1-10)
 *===========================================================================*/
iqs_error_e iqs_enable_specific_channels(uint8_t *tx_list, uint8_t tx_count,
                                          uint8_t *rx_list, uint8_t rx_count)
{
    iqs_error_e err;
    uint8_t buf[16];

    if (tx_list == NULL || rx_list == NULL) {
        return IQS_ERR_INVALID_PARAM;
    }
    if (tx_count == 0 || tx_count > 15 || rx_count == 0 || rx_count > 10) {
        LOG_ERR("Invalid count: Tx=%d (1-15), Rx=%d (1-10)", tx_count, rx_count);
        return IQS_ERR_INVALID_PARAM;
    }

    LOG_INFO("Enabling %d Tx and %d Rx channels...", tx_count, rx_count);

    // Set Total Rx
    buf[0] = rx_count;
    err = iqs_write_register(IQS_REG_TOTAL_RX, buf, 1);
    if (err != IQS_OK) return err;

    // Set Total Tx
    buf[0] = tx_count;
    err = iqs_write_register(IQS_REG_TOTAL_TX, buf, 1);
    if (err != IQS_OK) return err;

    // Set Rx Mapping
    err = iqs_write_register(IQS_REG_RX_MAPPING, rx_list, rx_count);
    if (err != IQS_OK) return err;

    // Set Tx Mapping
    err = iqs_write_register(IQS_REG_TX_MAPPING, tx_list, tx_count);
    if (err != IQS_OK) return err;

    // Log what was enabled
    LOG_INFO("Tx mapping: ");
    for (int i = 0; i < tx_count; i++) {
        LOG_INFO("  Logical Tx%d -> Physical Tx%d", i, tx_list[i]);
    }
    LOG_INFO("Rx mapping: ");
    for (int i = 0; i < rx_count; i++) {
        LOG_INFO("  Logical Rx%d -> Physical Rx%d", i, rx_list[i]);
    }

    return IQS_OK;
}

/*============================================================================
 * Enable Range of Channels
 *
 * Enables Tx[tx_start..tx_end] and Rx[rx_start..rx_end]
 *
 * @param tx_start: First Tx to enable (0-14)
 * @param tx_end: Last Tx to enable (0-14)
 * @param rx_start: First Rx to enable (0-9)
 * @param rx_end: Last Rx to enable (0-9)
 *===========================================================================*/
iqs_error_e iqs_enable_channel_range(uint8_t tx_start, uint8_t tx_end,
                                      uint8_t rx_start, uint8_t rx_end)
{
    if (tx_start > tx_end || tx_end > 14 || rx_start > rx_end || rx_end > 9) {
        LOG_ERR("Invalid range: Tx[%d-%d], Rx[%d-%d]", tx_start, tx_end, rx_start, rx_end);
        return IQS_ERR_INVALID_PARAM;
    }

    uint8_t tx_count = tx_end - tx_start + 1;
    uint8_t rx_count = rx_end - rx_start + 1;

    uint8_t tx_list[15];
    uint8_t rx_list[10];

    // Build Tx list
    for (int i = 0; i < tx_count; i++) {
        tx_list[i] = tx_start + i;
    }

    // Build Rx list
    for (int i = 0; i < rx_count; i++) {
        rx_list[i] = rx_start + i;
    }

    LOG_INFO("Enabling range: Tx[%d-%d], Rx[%d-%d]", tx_start, tx_end, rx_start, rx_end);

    return iqs_enable_specific_channels(tx_list, tx_count, rx_list, rx_count);
}

/*============================================================================
 * Enable ALL Channels (Full Matrix)
 *
 * Enables all 15 Tx and 10 Rx (150 channels total)
 *===========================================================================*/
iqs_error_e iqs_enable_all_channels(void)
{
    uint8_t tx_list[15] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
    uint8_t rx_list[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    LOG_INFO("Enabling ALL channels (15 Tx x 10 Rx = 150 total)");

    return iqs_enable_specific_channels(tx_list, 15, rx_list, 10);
}

/*============================================================================
 * Run ATI on Currently Enabled Channels
 *===========================================================================*/
iqs_error_e iqs_run_ati_current_channels(uint16_t ati_target)
{
    iqs_error_e err;
    uint8_t buf[2];

    LOG_INFO("Running ATI with target = %d...", ati_target);

    // Set ATI target (Big Endian)
    buf[0] = (ati_target >> 8) & 0xFF;
    buf[1] = ati_target & 0xFF;
    err = iqs_write_register(IQS_REG_ATI_TARGET, buf, 2);
    if (err != IQS_OK) {
        LOG_ERR("Failed to set ATI target");
        return err;
    }

    // Also set ALP ATI target
    iqs_write_register(0x056F, buf, 2);

    sl_udelay_wait(2000);  // 2ms

    // Trigger ATI
    buf[0] = 0x40;  // AUTO_ATI bit
    err = iqs_write_register(IQS_REG_SYS_CTRL0, buf, 1);
    if (err != IQS_OK) {
        LOG_ERR("Failed to trigger ATI");
        return err;
    }

    // Wait for completion
    uint32_t timeout = 0;
    uint8_t ctrl0;

    do {
        sl_udelay_wait(10000);  // 10ms
        timeout += 10;

        err = iqs_read_register(IQS_REG_SYS_CTRL0, &ctrl0, 1);
        if (err != IQS_OK) {
            LOG_ERR("Failed to read ATI status");
            return err;
        }

        if (timeout > 10000) {  // 10 second timeout
            LOG_ERR("ATI timeout!");
            return IQS_ERR_TIMEOUT;
        }
    } while (ctrl0 & 0x40);

    LOG_INFO("ATI completed in %lu ms", (unsigned long)timeout);

    // Check for errors
    uint8_t sys_info0;
    err = iqs_read_register(IQS_REG_SYS_INFO0, &sys_info0, 1);
    if (err != IQS_OK) return err;

    LOG_INFO("System Info 0 = 0x%02X", sys_info0);

    if (sys_info0 & 0x20) {
        LOG_ERR("ATI ERROR!");
        return IQS_ERR_NOT_INITIALIZED;
    }

    if (sys_info0 & 0x40) {
        LOG_WARN("ALP ATI ERROR (non-critical)");
    }

    LOG_INFO("ATI successful!");
    return IQS_OK;
}

/*============================================================================
 * Test Single Channel with ATI
 *
 * Complete test: enable one channel, run ATI, check result
 *===========================================================================*/
iqs_error_e iqs_test_single_channel_ati(uint8_t tx_num, uint8_t rx_num, uint16_t ati_target)
{
    iqs_error_e err;
    uint8_t buf[4];

    LOG_INFO("========================================");
    LOG_INFO("Testing Tx%d + Rx%d with ATI target %d", tx_num, rx_num, ati_target);
    LOG_INFO("========================================");

    // Step 1: Reset
    buf[0] = 0x02;
    iqs_write_register(IQS_REG_SYS_CTRL1, buf, 1);
    sl_udelay_wait(100000);  // 100ms

    // ACK Reset
    buf[0] = 0x80;
    iqs_write_register(IQS_REG_SYS_CTRL0, buf, 1);
    sl_udelay_wait(5000);

    // Step 2: Enable single channel
    err = iqs_enable_single_channel(tx_num, rx_num);
    if (err != IQS_OK) {
        LOG_ERR("Failed to enable channel");
        return err;
    }
    sl_udelay_wait(2000);

    // Step 3: Run ATI
    err = iqs_run_ati_current_channels(ati_target);
    if (err != IQS_OK) {
        LOG_ERR("ATI FAILED for Tx%d + Rx%d", tx_num, rx_num);
        return err;
    }

    // Step 4: Read count value
    err = iqs_read_register(0x0095, buf, 2);
    if (err == IQS_OK) {
        uint16_t count = (buf[0] << 8) | buf[1];
        LOG_INFO("Channel count value: %d", count);
    }

    LOG_INFO("SUCCESS: Tx%d + Rx%d works!", tx_num, rx_num);
    return IQS_OK;
}

/*============================================================================
 * Scan All Channels to Find Working Ones
 *
 * Tests each Tx with Rx0, then each Rx with Tx0
 * Returns bitmask of working channels
 *===========================================================================*/
iqs_error_e iqs_scan_all_channels(uint16_t *working_tx_mask, uint16_t *working_rx_mask)
{
    iqs_error_e err;
    uint8_t buf[4];
    uint16_t tx_mask = 0;
    uint16_t rx_mask = 0;

    LOG_INFO("==========================================");
    LOG_INFO("  Scanning ALL Tx/Rx Channels");
    LOG_INFO("==========================================");

    // Test each Tx with Rx0
    LOG_INFO("\n--- Testing Tx0-14 with Rx0 ---");

    for (uint8_t tx = 0; tx < 15; tx++) {
        // Reset
        buf[0] = 0x02;
        iqs_write_register(IQS_REG_SYS_CTRL1, buf, 1);
        sl_udelay_wait(50000);
        buf[0] = 0x80;
        iqs_write_register(IQS_REG_SYS_CTRL0, buf, 1);
        sl_udelay_wait(2000);

        // Enable single channel
        iqs_enable_single_channel(tx, 0);
        sl_udelay_wait(2000);

        // Run ATI with low target
//        err = iqs_run_ati_current_channels(300);
        err = iqs_auto_ATI_with_retry();

        if (err == IQS_OK) {
            tx_mask |= (1 << tx);
            LOG_INFO("  Tx%d + Rx0: OK", tx);
        } else {
            LOG_ERR("  Tx%d + Rx0: FAILED", tx);
        }
    }

    // Test each Rx with Tx0 (only if Tx0 works)
    LOG_INFO("\n--- Testing Rx0-9 with Tx0 ---");

    for (uint8_t rx = 0; rx < 10; rx++) {
        // Reset
        buf[0] = 0x02;
        iqs_write_register(IQS_REG_SYS_CTRL1, buf, 1);
        sl_udelay_wait(50000);
        buf[0] = 0x80;
        iqs_write_register(IQS_REG_SYS_CTRL0, buf, 1);
        sl_udelay_wait(2000);

        // Enable single channel
        iqs_enable_single_channel(0, rx);
        sl_udelay_wait(2000);

        // Run ATI
//        err = iqs_run_ati_current_channels(300);
        err = iqs_auto_ATI_with_retry();

        if (err == IQS_OK) {
            rx_mask |= (1 << rx);
            LOG_INFO("  Tx0 + Rx%d: OK", rx);
        } else {
            LOG_ERR("  Tx0 + Rx%d: FAILED", rx);
        }
    }

    // Summary
    LOG_INFO("\n==========================================");
    LOG_INFO("  SCAN RESULTS");
    LOG_INFO("==========================================");
    LOG_INFO("Working Tx mask: 0x%04X", tx_mask);
    LOG_INFO("Working Rx mask: 0x%04X", rx_mask);

    LOG_INFO("\nWorking Tx channels:");
    for (int i = 0; i < 15; i++) {
        if (tx_mask & (1 << i)) {
            LOG_INFO("  Tx%d: OK", i);
        }
    }

    LOG_INFO("\nWorking Rx channels:");
    for (int i = 0; i < 10; i++) {
        if (rx_mask & (1 << i)) {
            LOG_INFO("  Rx%d: OK", i);
        }
    }

    LOG_INFO("\nFailed Tx channels:");
    for (int i = 0; i < 15; i++) {
        if (!(tx_mask & (1 << i))) {
            LOG_ERR("  Tx%d: FAILED", i);
        }
    }

    LOG_INFO("\nFailed Rx channels:");
    for (int i = 0; i < 10; i++) {
        if (!(rx_mask & (1 << i))) {
            LOG_ERR("  Rx%d: FAILED", i);
        }
    }

    if (working_tx_mask) *working_tx_mask = tx_mask;
    if (working_rx_mask) *working_rx_mask = rx_mask;

    return IQS_OK;
}

/*============================================================================
 * Quick Test: Just One Channel
 *===========================================================================*/
iqs_error_e iqs_quick_channel_test(uint8_t tx, uint8_t rx)
{
    return iqs_test_single_channel_ati(tx, rx, 800);
}

/*============================================================================
 * Initialize with Only Working Channels
 *
 * After scanning, use this to init with only channels that passed
 *===========================================================================*/
iqs_error_e iqs_init_with_working_channels(uint16_t tx_mask, uint16_t rx_mask)
{
    uint8_t tx_list[15];
    uint8_t rx_list[10];
    uint8_t tx_count = 0;
    uint8_t rx_count = 0;

    // Build Tx list from mask
    for (int i = 0; i < 15; i++) {
        if (tx_mask & (1 << i)) {
            tx_list[tx_count++] = i;
        }
    }

    // Build Rx list from mask
    for (int i = 0; i < 10; i++) {
        if (rx_mask & (1 << i)) {
            rx_list[rx_count++] = i;
        }
    }

    if (tx_count == 0 || rx_count == 0) {
        LOG_ERR("No working channels found!");
        return IQS_ERR_NOT_INITIALIZED;
    }

    LOG_INFO("Initializing with %d Tx and %d Rx channels", tx_count, rx_count);

    // Enable these channels
    iqs_error_e err = iqs_enable_specific_channels(tx_list, tx_count, rx_list, rx_count);
    if (err != IQS_OK) return err;

    // Run ATI
    err = iqs_run_ati_current_channels(500);
    if (err != IQS_OK) return err;

    // Configure event mode
    uint8_t buf[2];
    buf[0] = 0x07;  // TP_EVENT | GESTURE_EVENT | EVENT_MODE
    iqs_write_register(0x058F, buf, 1);

    buf[0] = 0x40;  // SETUP_COMPLETE
    iqs_write_register(0x058E, buf, 1);

    // ACK reset
    buf[0] = 0x80;
    iqs_write_register(IQS_REG_SYS_CTRL0, buf, 1);

    LOG_INFO("Initialization complete with working channels only");
    return IQS_OK;
}
