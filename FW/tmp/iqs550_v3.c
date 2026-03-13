/*
 * iqs550.c
 *
 * IQS550-B000 Touch Controller Driver for Custom PCB
 * Configuration: Tx(0-14) x Rx(0-9) = 150 channels
 *
 * Based on original iqs572.c by Bhakti Ramani
 * Modified for IQS550 custom PCB
 */

#include "iqs550_V3.h"

/*============================================================================
 * Error String Conversion
 *===========================================================================*/
const char* iqs_error_to_str(iqs_error_e err)
{
    switch (err)
    {
        case IQS_OK:                     return "IQS_OK";
        case IQS_ERR_I2C_READ_FAIL:      return "IQS_ERR_I2C_READ_FAIL";
        case IQS_ERR_I2C_WRITE_FAIL:     return "IQS_ERR_I2C_WRITE_FAIL";
        case IQS_ERR_INVALID_PARAM:      return "IQS_ERR_INVALID_PARAM";
        case IQS_ERR_VERSION_CHECK_FAIL: return "IQS_ERR_VERSION_CHECK_FAIL";
        case IQS_ERR_NOT_INITIALIZED:    return "IQS_ERR_NOT_INITIALIZED";
        case IQS_ERR_TIMEOUT:            return "IQS_ERR_TIMEOUT";
        case IQS_ERR_NO_TOUCH:           return "IQS_ERR_NO_TOUCH";
        case IQS_GESTURE_EN_FAIL:        return "IQS_GESTURE_EN_FAIL";
        default:                         return "IQS_ERR_UNKNOWN";
    }
}

/*============================================================================
 * Internal I2C Helper Functions
 *===========================================================================*/
static iqs_error_e iqs_read_register(uint16_t reg_addr, uint8_t *data, size_t len)
{
    if (data == NULL || len == 0)
    {
        LOG_ERR("Invalid parameters: data=%p, len=%zu", data, len);
        return IQS_ERR_INVALID_PARAM;
    }

    I2C_TransferReturn_TypeDef status = i2c_read(IQS_I2C_ADDR, reg_addr, data, len);

    if (status != i2cTransferDone)
    {
        LOG_ERR("I2C read failed at address 0x%04X, status=%d", reg_addr, status);
        return IQS_ERR_I2C_READ_FAIL;
    }

    LOG_DBG("Read %zu bytes from 0x%04X", len, reg_addr);
    return IQS_OK;
}

static iqs_error_e iqs_write_register(uint16_t reg_addr, uint8_t *data, size_t len)
{
    if (data == NULL || len == 0)
    {
        LOG_ERR("Invalid parameters: data=%p, len=%zu", data, len);
        return IQS_ERR_INVALID_PARAM;
    }

    I2C_TransferReturn_TypeDef status = i2c_write(IQS_I2C_ADDR, reg_addr, data, len);

    if (status != i2cTransferDone)
    {
        LOG_ERR("I2C write failed at address 0x%04X, status=%d", reg_addr, status);
        return IQS_ERR_I2C_WRITE_FAIL;
    }

    LOG_DBG("Wrote %zu bytes to 0x%04X", len, reg_addr);
    return IQS_OK;
}

/*============================================================================
 * System Control Functions
 *===========================================================================*/
iqs_error_e iqs_ack_reset(void)
{
    iqs_error_e err;
    uint8_t reset_cmd = IQS_SYS_CTRL0_ACK_RESET;  // 0x80

    LOG_INFO("Acknowledging IQS5xx reset...");

    err = iqs_write_register(IQS_REG_SYS_CONTROL_0, &reset_cmd, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to acknowledge system reset: %s", iqs_error_to_str(err));
        return err;
    }

    LOG_INFO("Reset acknowledged successfully");
    return IQS_OK;
}

iqs_error_e iqs_reset(void)
{
    iqs_error_e err;
    uint8_t reset_cmd = IQS_SYS_CTRL1_RESET;  // 0x02

    LOG_INFO("Initiating IQS5xx software reset...");

    err = iqs_write_register(IQS_REG_SYS_CONTROL_1, &reset_cmd, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to initiate system reset: %s", iqs_error_to_str(err));
        return err;
    }

    LOG_INFO("Software reset initiated successfully");

    // Wait for reset to complete
    sl_udelay_wait(10000);  // 10ms delay

    return IQS_OK;
}

iqs_error_e iqs_suspend(void)
{
    iqs_error_e err;
    uint8_t suspend_cmd = IQS_SYS_CTRL1_SUSPEND;  // 0x01

    LOG_INFO("Entering suspend mode...");

    err = iqs_write_register(IQS_REG_SYS_CONTROL_1, &suspend_cmd, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to enter suspend: %s", iqs_error_to_str(err));
        return err;
    }

    return IQS_OK;
}

iqs_error_e iqs_resume(void)
{
    iqs_error_e err;
    uint8_t resume_cmd = 0x00;

    LOG_INFO("Resuming from suspend...");

    err = iqs_write_register(IQS_REG_SYS_CONTROL_1, &resume_cmd, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to resume: %s", iqs_error_to_str(err));
        return err;
    }

    return IQS_OK;
}

/*============================================================================
 * Auto ATI (Auto-Tuning Implementation)
 *===========================================================================*/
iqs_error_e iqs_auto_ATI(void)
{
    iqs_error_e err;

    uint8_t sys_control_flags = IQS_SYS_CTRL0_AUTO_ATI;  // 0x40
    err = iqs_write_register(IQS_REG_SYS_CONTROL_0, &sys_control_flags, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to write system control auto ATI flag: %s", iqs_error_to_str(err));
        return err;
    }
    LOG_INFO("Auto ATI Bit successfully set, waiting for ATI to complete...");

    uint8_t ati_status;
    uint32_t timeout_count = 0;
    const uint32_t max_timeout = 2000;  // ~6 seconds with 3ms delay

    do {
        sl_udelay_wait(3000);  // 3ms
        err = iqs_read_register(IQS_REG_SYS_CONTROL_0, &ati_status, 1);
        if (err != IQS_OK)
        {
            LOG_ERR("Failed to read system control auto ATI flag: %s", iqs_error_to_str(err));
            return err;
        }
        LOG_DBG("ATI status: 0x%02X", ati_status);
        timeout_count++;

        if (timeout_count >= max_timeout)
        {
            LOG_ERR("ATI timeout after %lu iterations", (unsigned long)timeout_count);
            return IQS_ERR_TIMEOUT;
        }
    } while (ati_status & IQS_SYS_CTRL0_AUTO_ATI);

    // Check for ATI error in System Info 0
    uint8_t sys_info0;
    err = iqs_read_register(IQS_REG_SYSTEM_INFO_0, &sys_info0, 1);
    if (err == IQS_OK && (sys_info0 & IQS_SYS_INFO0_ATI_ERROR))
    {
        LOG_ERR("ATI error flag set in System Info 0: 0x%02X", sys_info0);
        return IQS_ERR_NOT_INITIALIZED;
    }

    LOG_INFO("ATI calibration completed! Status: 0x%02X", ati_status);
    return IQS_OK;
}

/*============================================================================
 * System Configuration
 *===========================================================================*/
iqs_error_e iqs_sys_config(void)
{
    iqs_error_e err;

    LOG_INFO("Configuring system (Event Mode + TP Event)...");

    // Read current SYS_CONFIG_1 value
    uint8_t current_val = 0x00;
    err = iqs_read_register(IQS_REG_SYS_CONFIG_1, &current_val, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to read system config 1: %s", iqs_error_to_str(err));
        return err;
    }
    LOG_DBG("Current SYS_CONFIG_1: 0x%02X", current_val);

    // Configure: Event Mode + Trackpad Event
    uint8_t sys_config = IQS_SYS_CFG1_TP_EVENT | IQS_SYS_CFG1_EVENT_MODE;  // 0x05

    // Optionally add gesture events
    sys_config |= IQS_SYS_CFG1_GESTURE_EVENT;  // 0x07

    err = iqs_write_register(IQS_REG_SYS_CONFIG_1, &sys_config, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to write system config: %s", iqs_error_to_str(err));
        return err;
    }

    LOG_INFO("System config written: 0x%02X", sys_config);
    sl_udelay_wait(2000);  // 2ms

    // Verify configuration
    LOG_INFO("Verifying system config...");
    uint8_t sys_config_readback = 0x00;
    err = iqs_read_register(IQS_REG_SYS_CONFIG_1, &sys_config_readback, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to read back system config: %s", iqs_error_to_str(err));
        return err;
    }

    LOG_INFO("System config readback: 0x%02X", sys_config_readback);

    if ((sys_config_readback & IQS_SYS_CFG1_EVENT_MODE) == 0)
    {
        LOG_ERR("EVENT_MODE bit not set! Device may not be in event mode");
        return IQS_ERR_NOT_INITIALIZED;
    }

    if ((sys_config_readback & IQS_SYS_CFG1_TP_EVENT) == 0)
    {
        LOG_ERR("TP_EVENT bit not set! Device may not report trackpad events");
        return IQS_ERR_NOT_INITIALIZED;
    }

    // Set SETUP_COMPLETE in SYS_CONFIG_0
    LOG_INFO("Setting Setup Complete flag...");
    uint8_t sys_config_0 = IQS_SYS_CFG0_SETUP_COMPLETE;  // 0x40
    err = iqs_write_register(IQS_REG_SYS_CONFIG_0, &sys_config_0, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to write setup complete: %s", iqs_error_to_str(err));
        return err;
    }

    LOG_INFO("Configuration verified successfully");
    return IQS_OK;
}

iqs_error_e iqs_read_sys_flags(void)
{
    iqs_error_e err;
    uint8_t sys_flags[2] = {0};

    err = iqs_read_register(IQS_REG_SYS_CONFIG_1, sys_flags, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to read system flags: %s", iqs_error_to_str(err));
        return err;
    }

    LOG_INFO("System flags: 0x%02X", sys_flags[0]);
    return IQS_OK;
}

/*============================================================================
 * Gesture Enable/Disable
 *===========================================================================*/
iqs_error_e iqs_gesture_enable(void)
{
    iqs_error_e err;

    LOG_INFO("Enabling gestures...");

    // Enable single finger gestures
    uint8_t single_gesture_enable = IQS_SINGLE_FINGER_GESTURE_EN_MASK;  // 0x3F
    err = iqs_write_register(IQS_REG_SINGLE_FINGER_GESTURE_EN, &single_gesture_enable, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to write single finger gesture enable: %s", iqs_error_to_str(err));
        return err;
    }

    // Enable multi-finger gestures
    uint8_t multi_gesture_enable = IQS_MULTI_FINGER_GESTURE_EN_MASK;  // 0x07
    err = iqs_write_register(IQS_REG_MULTI_FINGER_GESTURE_EN, &multi_gesture_enable, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to write multi finger gesture enable: %s", iqs_error_to_str(err));
        return err;
    }

    sl_udelay_wait(2000);  // 2ms

    // Verify
    uint8_t single_readback = 0x00;
    uint8_t multi_readback = 0x00;

    err = iqs_read_register(IQS_REG_SINGLE_FINGER_GESTURE_EN, &single_readback, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to read back single gesture enable: %s", iqs_error_to_str(err));
        return err;
    }

    err = iqs_read_register(IQS_REG_MULTI_FINGER_GESTURE_EN, &multi_readback, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to read back multi gesture enable: %s", iqs_error_to_str(err));
        return err;
    }

    LOG_INFO("Gesture EN registers: Single=0x%02X, Multi=0x%02X", single_readback, multi_readback);

    if ((single_readback & IQS_SINGLE_FINGER_GESTURE_EN_MASK) == 0)
    {
        LOG_ERR("Single finger gestures not enabled");
        return IQS_GESTURE_EN_FAIL;
    }

    if ((multi_readback & IQS_MULTI_FINGER_GESTURE_EN_MASK) == 0)
    {
        LOG_ERR("Multi finger gestures not enabled");
        return IQS_GESTURE_EN_FAIL;
    }

    LOG_INFO("Gestures enabled successfully");
    return IQS_OK;
}

iqs_error_e iqs_gesture_disable(void)
{
    iqs_error_e err;

    LOG_INFO("Disabling gestures...");

    uint8_t disable = 0x00;

    err = iqs_write_register(IQS_REG_SINGLE_FINGER_GESTURE_EN, &disable, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to disable single finger gestures: %s", iqs_error_to_str(err));
        return err;
    }

    err = iqs_write_register(IQS_REG_MULTI_FINGER_GESTURE_EN, &disable, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to disable multi finger gestures: %s", iqs_error_to_str(err));
        return err;
    }

    sl_udelay_wait(2000);  // 2ms

    LOG_INFO("Gestures disabled successfully");
    return IQS_OK;
}

/*============================================================================
 * Channel Configuration - FOR CUSTOM PCB (Tx 0-14, Rx 0-9)
 *===========================================================================*/

/**
 * @brief Get default channel configuration for custom PCB
 */
void iqs_get_default_channel_config(iqs_channel_config_t *config)
{
    if (config == NULL) return;

    memset(config, 0, sizeof(iqs_channel_config_t));

    config->total_rx = IQS_RX_SIZE;  // 10
    config->total_tx = IQS_TX_SIZE;  // 15

    // Default 1:1 mapping for Rx (0-9)
    for (uint8_t i = 0; i < IQS_RX_SIZE; i++)
    {
        config->rx_mapping[i] = i;
    }

    // Default 1:1 mapping for Tx (0-14)
    for (uint8_t i = 0; i < IQS_TX_SIZE; i++)
    {
        config->tx_mapping[i] = i;
    }

    // Default resolution
    config->x_resolution = IQS_DEFAULT_X_RES;  // 3584
    config->y_resolution = IQS_DEFAULT_Y_RES;  // 2304

    // No axis flip/swap by default
    config->xy_config = 0x00;
}

/**
 * @brief Set total Tx and Rx channel counts
 */
iqs_error_e iqs_set_total_tx_rx(void)
{
    iqs_error_e err;

    LOG_INFO("Setting Tx/Rx counts: Tx=%d, Rx=%d", IQS_TX_SIZE, IQS_RX_SIZE);

    // Write Total Rx (0x063D)
    uint8_t total_rx = IQS_RX_SIZE;
    err = iqs_write_register(IQS_REG_TOTAL_RX, &total_rx, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set Total Rx: %s", iqs_error_to_str(err));
        return err;
    }

    // Write Total Tx (0x063E)
    uint8_t total_tx = IQS_TX_SIZE;
    err = iqs_write_register(IQS_REG_TOTAL_TX, &total_tx, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set Total Tx: %s", iqs_error_to_str(err));
        return err;
    }

    // Verify
    uint8_t rx_readback = 0, tx_readback = 0;
    err = iqs_read_register(IQS_REG_TOTAL_RX, &rx_readback, 1);
    if (err == IQS_OK)
    {
        err = iqs_read_register(IQS_REG_TOTAL_TX, &tx_readback, 1);
    }

    if (err == IQS_OK)
    {
        LOG_INFO("Tx/Rx verified: Tx=%d, Rx=%d", tx_readback, rx_readback);
    }

    return IQS_OK;
}

/**
 * @brief Set Rx channel mapping
 */
iqs_error_e iqs_set_rx_mapping(const uint8_t *mapping, uint8_t len)
{
    if (mapping == NULL || len == 0 || len > 10)
    {
        return IQS_ERR_INVALID_PARAM;
    }

    LOG_INFO("Setting Rx mapping (%d channels)...", len);

    iqs_error_e err = iqs_write_register(IQS_REG_RX_MAPPING, (uint8_t *)mapping, len);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set Rx mapping: %s", iqs_error_to_str(err));
        return err;
    }

    return IQS_OK;
}

/**
 * @brief Set Tx channel mapping
 */
iqs_error_e iqs_set_tx_mapping(const uint8_t *mapping, uint8_t len)
{
    if (mapping == NULL || len == 0 || len > 15)
    {
        return IQS_ERR_INVALID_PARAM;
    }

    LOG_INFO("Setting Tx mapping (%d channels)...", len);

    iqs_error_e err = iqs_write_register(IQS_REG_TX_MAPPING, (uint8_t *)mapping, len);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set Tx mapping: %s", iqs_error_to_str(err));
        return err;
    }

    return IQS_OK;
}

/**
 * @brief Set X and Y resolution
 */
iqs_error_e iqs_set_resolution(uint16_t x_res, uint16_t y_res)
{
    iqs_error_e err;
    uint8_t buf[2];

    LOG_INFO("Setting resolution: X=%d, Y=%d", x_res, y_res);

    // X Resolution (0x066C) - Big-endian
    buf[0] = (x_res >> 8) & 0xFF;
    buf[1] = x_res & 0xFF;
    err = iqs_write_register(IQS_REG_X_RESOLUTION, buf, 2);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set X resolution: %s", iqs_error_to_str(err));
        return err;
    }

    // Y Resolution (0x066E) - Big-endian
    buf[0] = (y_res >> 8) & 0xFF;
    buf[1] = y_res & 0xFF;
    err = iqs_write_register(IQS_REG_Y_RESOLUTION, buf, 2);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set Y resolution: %s", iqs_error_to_str(err));
        return err;
    }

    return IQS_OK;
}

/**
 * @brief Set XY configuration (axis flip/swap)
 */
iqs_error_e iqs_set_xy_config(uint8_t config)
{
    LOG_INFO("Setting XY config: 0x%02X", config);
    return iqs_write_register(IQS_REG_XY_CONFIG, &config, 1);
}

/**
 * @brief Configure all channel parameters
 */
iqs_error_e iqs_configure_channels(const iqs_channel_config_t *config)
{
    if (config == NULL)
    {
        return IQS_ERR_INVALID_PARAM;
    }

    iqs_error_e err;

    LOG_INFO("=== Configuring channels for custom PCB ===");
    LOG_INFO("Tx channels: %d, Rx channels: %d", config->total_tx, config->total_rx);

    // Step 1: Set total Rx and Tx counts
    uint8_t total_rx = config->total_rx;
    uint8_t total_tx = config->total_tx;

    err = iqs_write_register(IQS_REG_TOTAL_RX, &total_rx, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set Total Rx");
        return err;
    }

    err = iqs_write_register(IQS_REG_TOTAL_TX, &total_tx, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set Total Tx");
        return err;
    }

    // Step 2: Set Rx mapping (0x063F, 10 bytes)
    err = iqs_write_register(IQS_REG_RX_MAPPING, (uint8_t *)config->rx_mapping, config->total_rx);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set Rx mapping");
        return err;
    }
    LOG_DBG("Rx mapping set");

    // Step 3: Set Tx mapping (0x0649, 15 bytes)
    err = iqs_write_register(IQS_REG_TX_MAPPING, (uint8_t *)config->tx_mapping, config->total_tx);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set Tx mapping");
        return err;
    }
    LOG_DBG("Tx mapping set");

    // Step 4: Set resolution
    err = iqs_set_resolution(config->x_resolution, config->y_resolution);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set resolution");
        return err;
    }

    // Step 5: Set XY config (axis orientation)
    err = iqs_set_xy_config(config->xy_config);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set XY config");
        return err;
    }

    LOG_INFO("Channel configuration complete");
    return IQS_OK;
}

/*============================================================================
 * Device Information
 *===========================================================================*/
iqs_error_e iqs_get_device_info(iqs_device_info_t *info)
{
    iqs_error_e err;
    uint8_t data[IQS_DEVICE_INFO_SIZE] = {0};

    if (info == NULL)
    {
        LOG_ERR("Invalid parameter: info pointer is NULL");
        return IQS_ERR_INVALID_PARAM;
    }

    LOG_DBG("Reading device info from register 0x%04X...", IQS_REG_DEVICE_INFO);

    err = iqs_read_register(IQS_REG_DEVICE_INFO, data, IQS_DEVICE_INFO_SIZE);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to read device info: %s", iqs_error_to_str(err));
        return err;
    }

    // Parse device info (Big-endian format)
    info->product_num = (data[0] << 8) | data[1];   // Bytes 0-1: Product Number
    info->project_num = (data[2] << 8) | data[3];   // Bytes 2-3: Project Number
    info->major_ver = data[4];                       // Byte 4: Major version
    info->minor_ver = data[5];                       // Byte 5: Minor version

    LOG_INFO("Raw bytes: %02X %02X %02X %02X %02X %02X",
             data[0], data[1], data[2], data[3], data[4], data[5]);
    LOG_INFO("Device Info Retrieved:");
    LOG_INFO("  Product Number: %d (0x%04X)", info->product_num, info->product_num);
    LOG_INFO("  Project Number: %d (0x%04X)", info->project_num, info->project_num);
    LOG_INFO("  Firmware Version: %d.%d", info->major_ver, info->minor_ver);

    // Validate product number (IQS550 or IQS572)
    bool valid_product = (info->product_num == IQS550_PRODUCT_NUM) ||
                         (info->product_num == IQS572_PRODUCT_NUM);

    if (!valid_product)
    {
        LOG_ERR("Invalid product number! Expected %d or %d, got %d",
                IQS550_PRODUCT_NUM, IQS572_PRODUCT_NUM, info->product_num);
        return IQS_ERR_VERSION_CHECK_FAIL;
    }

    // Validate project number (B000 = 15)
    if (info->project_num != IQS_PROJECT_NUM)
    {
        LOG_WARN("Project number mismatch! Expected %d (B000), got %d",
                 IQS_PROJECT_NUM, info->project_num);
        // Don't fail, just warn - might be A000 firmware
    }

    LOG_INFO("Device identification successful");
    return IQS_OK;
}

/*============================================================================
 * Touch Data Reading
 *===========================================================================*/
iqs_error_e iqs_get_rel_coordinates(iqs_coordinates_t *coords)
{
    iqs_error_e err;
    uint8_t coord_data[4] = {0};

    if (coords == NULL)
    {
        LOG_ERR("Invalid parameter: coords pointer is NULL");
        return IQS_ERR_INVALID_PARAM;
    }

    memset(coords, 0, sizeof(iqs_coordinates_t));

    // Read Relative X and Y (0x0012-0x0015)
    err = iqs_read_register(IQS_REG_REL_X, coord_data, 4);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to read relative coordinates: %s", iqs_error_to_str(err));
        return err;
    }

    // Parse as big-endian signed 16-bit
    coords->x = (int16_t)((coord_data[0] << 8) | coord_data[1]);
    coords->y = (int16_t)((coord_data[2] << 8) | coord_data[3]);

    LOG_DBG("Relative coords: X=%d, Y=%d", coords->x, coords->y);

    return IQS_OK;
}

iqs_error_e iqs_get_abs_coordinates(iqs_coordinates_t *coords)
{
    iqs_error_e err;
    uint8_t coord_data[4] = {0};

    if (coords == NULL)
    {
        LOG_ERR("Invalid parameter: coords pointer is NULL");
        return IQS_ERR_INVALID_PARAM;
    }

    memset(coords, 0, sizeof(iqs_coordinates_t));

    // Read Absolute X and Y (0x0016-0x0019)
    err = iqs_read_register(IQS_REG_ABS_X, coord_data, 4);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to read absolute coordinates: %s", iqs_error_to_str(err));
        return err;
    }

    // Parse as big-endian unsigned 16-bit
    coords->x = (coord_data[0] << 8) | coord_data[1];
    coords->y = (coord_data[2] << 8) | coord_data[3];

    LOG_DBG("Absolute coords: X=%d, Y=%d", coords->x, coords->y);

    return IQS_OK;
}

iqs_error_e iqs_read_all_events(iqs_full_events_t *events)
{
    iqs_error_e err;
    uint8_t event_buffer[16] = {0};  // Read gesture events through touch data

    if (events == NULL)
    {
        LOG_ERR("Invalid parameter: events pointer is NULL");
        return IQS_ERR_INVALID_PARAM;
    }

    memset(events, 0, sizeof(iqs_full_events_t));

    // Read from 0x000D (Gesture Events 0) through touch coordinates
    // 0x000D: Gesture Events 0
    // 0x000E: Gesture Events 1
    // 0x000F: System Info 0
    // 0x0010: System Info 1
    // 0x0011: Num Fingers
    // 0x0012-0x0013: Rel X
    // 0x0014-0x0015: Rel Y
    err = iqs_read_register(IQS_REG_GESTURE_EVENTS_0, event_buffer, 9);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to read event block: %s", iqs_error_to_str(err));
        return err;
    }

    // Parse the data
    events->gesture_events_0 = event_buffer[0];     // 0x000D
    events->gesture_events_1 = event_buffer[1];     // 0x000E
    events->system_info_0 = event_buffer[2];        // 0x000F
    events->system_info_1 = event_buffer[3];        // 0x0010
    events->num_fingers = event_buffer[4];          // 0x0011
    events->rel_x = (int16_t)((event_buffer[5] << 8) | event_buffer[6]);  // 0x0012-0x0013
    events->rel_y = (int16_t)((event_buffer[7] << 8) | event_buffer[8]);  // 0x0014-0x0015

    // Initialize gesture flags
    events->single_tap = false;
    events->press_and_hold = false;
    events->double_tap = false;
    events->scroll = false;
    events->zoom = false;

    LOG_DBG("System Info: 0=0x%02X, 1=0x%02X", events->system_info_0, events->system_info_1);
    LOG_DBG("Gestures: 0=0x%02X, 1=0x%02X", events->gesture_events_0, events->gesture_events_1);
    LOG_DBG("Touch: Fingers=%d, RelX=%d, RelY=%d", events->num_fingers, events->rel_x, events->rel_y);

    // Decode single finger gestures
    if (events->gesture_events_0 & IQS_GESTURE_SINGLE_TAP)
    {
        events->single_tap = true;
        LOG_INFO("Gesture: Single Tap");
    }
    if (events->gesture_events_0 & IQS_GESTURE_PRESS_AND_HOLD)
    {
        events->press_and_hold = true;
        LOG_INFO("Gesture: Press and Hold");
    }
    if (events->gesture_events_0 & IQS_GESTURE_SWIPE_X_NEG)
    {
        LOG_INFO("Gesture: Swipe X-");
    }
    if (events->gesture_events_0 & IQS_GESTURE_SWIPE_X_POS)
    {
        LOG_INFO("Gesture: Swipe X+");
    }
    if (events->gesture_events_0 & IQS_GESTURE_SWIPE_Y_POS)
    {
        LOG_INFO("Gesture: Swipe Y+");
    }
    if (events->gesture_events_0 & IQS_GESTURE_SWIPE_Y_NEG)
    {
        LOG_INFO("Gesture: Swipe Y-");
    }

    // Decode multi-finger gestures
    if (events->gesture_events_1 & IQS_GESTURE_2_FINGER_TAP)
    {
        events->double_tap = true;
        LOG_INFO("Gesture: 2-Finger Tap");
    }
    if (events->gesture_events_1 & IQS_GESTURE_SCROLL)
    {
        events->scroll = true;
        LOG_INFO("Gesture: Scroll");
    }
    if (events->gesture_events_1 & IQS_GESTURE_ZOOM)
    {
        events->zoom = true;
        LOG_INFO("Gesture: Zoom");
    }

    return IQS_OK;
}

/*============================================================================
 * Raw Data Reading
 *===========================================================================*/
iqs_error_e iqs_read_delta_values(uint8_t *delta_buf)
{
    if (delta_buf == NULL)
    {
        return IQS_ERR_INVALID_PARAM;
    }

    iqs_error_e err = iqs_read_register(IQS_REG_DELTA_VALUES, delta_buf, IQS_DELTA_LENGTH);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to read delta values: %s", iqs_error_to_str(err));
        return err;
    }

    return IQS_OK;
}

iqs_error_e iqs_read_count_values(uint8_t *count_buf)
{
    if (count_buf == NULL)
    {
        return IQS_ERR_INVALID_PARAM;
    }

    iqs_error_e err = iqs_read_register(IQS_REG_COUNT_VALUES, count_buf, IQS_COUNT_LENGTH);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to read count values: %s", iqs_error_to_str(err));
        return err;
    }

    return IQS_OK;
}

/*============================================================================
 * Communication Control
 *===========================================================================*/
iqs_error_e iqs_end_of_communication(void)
{
    iqs_error_e err;
    uint8_t end_byte = 0x00;

    err = iqs_write_register(IQS_END_OF_COMMUNICATION, &end_byte, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to end communication: %s", iqs_error_to_str(err));
        return err;
    }

    sl_udelay_wait(500);  // 500μs delay

    return IQS_OK;
}

/*============================================================================
 * Initialization Functions
 *===========================================================================*/

/**
 * @brief Standard initialization (for IQS572 dev kit or similar)
 */
iqs_error_e iqs_init(void)
{
    iqs_device_info_t info = {0};
    iqs_error_e err;

    LOG_INFO("=== Starting IQS5xx Initialization ===");

    // Step 1: Software Reset
    LOG_INFO("Step 1: Software reset...");
    err = iqs_reset();
    if (err != IQS_OK)
    {
        LOG_ERR("Reset failed: %s", iqs_error_to_str(err));
        return err;
    }
    sl_udelay_wait(5000);  // 5ms

    // Step 2: Read device info
    LOG_INFO("Step 2: Reading device information...");
    err = iqs_get_device_info(&info);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to get device info: %s", iqs_error_to_str(err));
        return err;
    }
    LOG_INFO("Device: Product=%d, Project=%d, FW=%d.%d",
             info.product_num, info.project_num, info.major_ver, info.minor_ver);

    // Step 3: Auto ATI
    LOG_INFO("Step 3: Auto ATI...");
    err = iqs_auto_ATI();
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to run Auto ATI: %s", iqs_error_to_str(err));
        return err;
    }

    // Step 4: Acknowledge reset after ATI
    LOG_INFO("Step 4: Acknowledge Reset...");
    err = iqs_ack_reset();
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to acknowledge reset: %s", iqs_error_to_str(err));
        return err;
    }

    // Step 5: Configure system (event mode, etc.)
    LOG_INFO("Step 5: System configuration...");
    err = iqs_sys_config();
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to configure system: %s", iqs_error_to_str(err));
        return err;
    }

    // Step 6: Enable gestures
    LOG_INFO("Step 6: Enable gestures...");
    err = iqs_gesture_enable();
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to enable gestures: %s", iqs_error_to_str(err));
        return err;
    }

    LOG_INFO("=== IQS5xx Initialization Complete ===");
    return IQS_OK;
}

/**
 * @brief Initialization for custom PCB with channel configuration
 */
iqs_error_e iqs_init_custom_pcb(const iqs_channel_config_t *config)
{
    iqs_device_info_t info = {0};
    iqs_channel_config_t default_config;
    const iqs_channel_config_t *cfg;
    iqs_error_e err;

    LOG_INFO("=== Starting IQS550 Custom PCB Initialization ===");
    LOG_INFO("Configuration: Tx(0-14) x Rx(0-9) = 150 channels");

    // Use provided config or defaults
    if (config == NULL)
    {
        iqs_get_default_channel_config(&default_config);
        cfg = &default_config;
        LOG_INFO("Using default channel configuration");
    }
    else
    {
        cfg = config;
        LOG_INFO("Using custom channel configuration");
    }

    // Step 1: Software Reset
    LOG_INFO("Step 1: Software reset...");
    err = iqs_reset();
    if (err != IQS_OK)
    {
        LOG_ERR("Reset failed: %s", iqs_error_to_str(err));
        return err;
    }
    sl_udelay_wait(10000);  // 10ms for reset

    // Step 2: Read and verify device info
    LOG_INFO("Step 2: Reading device information...");
    err = iqs_get_device_info(&info);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to get device info: %s", iqs_error_to_str(err));
        return err;
    }

    // Verify it's an IQS550
    if (info.product_num != IQS550_PRODUCT_NUM)
    {
        LOG_WARN("Expected IQS550 (product=%d), got product=%d",
                 IQS550_PRODUCT_NUM, info.product_num);
    }

    // Step 3: Configure Tx/Rx channels
    LOG_INFO("Step 3: Configuring Tx/Rx channels...");
    err = iqs_configure_channels(cfg);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to configure channels: %s", iqs_error_to_str(err));
        return err;
    }

    // Step 4: Run Auto ATI
    LOG_INFO("Step 4: Auto ATI...");
    err = iqs_auto_ATI();
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to run Auto ATI: %s", iqs_error_to_str(err));
        return err;
    }

    // Step 5: Acknowledge reset
    LOG_INFO("Step 5: Acknowledge Reset...");
    err = iqs_ack_reset();
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to acknowledge reset: %s", iqs_error_to_str(err));
        return err;
    }

    // Step 6: Configure system (event mode)
    LOG_INFO("Step 6: System configuration...");
    err = iqs_sys_config();
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to configure system: %s", iqs_error_to_str(err));
        return err;
    }

    // Step 7: Enable gestures
    LOG_INFO("Step 7: Enable gestures...");
    err = iqs_gesture_enable();
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to enable gestures: %s", iqs_error_to_str(err));
        return err;
    }

    LOG_INFO("=== IQS550 Custom PCB Initialization Complete ===");
    LOG_INFO("Ready for touch detection on %d channels", IQS_TOTAL_CHANNELS);

    return IQS_OK;
}