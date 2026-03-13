/**
 * @file iqs550_driver.c
 * @brief IQS550 Touch Controller Driver Implementation for EFR32
 * @details Initialization and configuration for custom PCB with Tx(0-14), Rx(0-9)
 */

#include "app.h"
#include <string.h>


/*============================================================================
 * Platform-specific delay function
 *===========================================================================*/
extern void delay_ms(uint32_t ms);  // Implement this for your platform

/*============================================================================
 * I2C Low-level functions (implement these for your EFR32)
 *===========================================================================*/
extern I2C_TransferReturn_TypeDef i2c_read(uint8_t addr, uint16_t reg, uint8_t *data, size_t len);
extern I2C_TransferReturn_TypeDef i2c_write(uint8_t addr, uint16_t reg, uint8_t *data, size_t len);

// For platforms that use different I2C return types
typedef enum {
    i2cTransferDone = 0,
    i2cTransferNack,
    i2cTransferBusErr,
    i2cTransferArbLost,
    i2cTransferUsageFault,
    i2cTransferSwFault
} I2C_TransferReturn_TypeDef;

/*============================================================================
 * Internal Helper Functions - I2C Register Access
 *===========================================================================*/

/**
 * @brief Read from IQS550 register
 */
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

/**
 * @brief Write to IQS550 register
 */
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

/**
 * @brief Read single byte from register
 */
static iqs_error_e iqs_read_byte(uint16_t reg_addr, uint8_t *value)
{
    return iqs_read_register(reg_addr, value, 1);
}

/**
 * @brief Write single byte to register
 */
static iqs_error_e iqs_write_byte(uint16_t reg_addr, uint8_t value)
{
    return iqs_write_register(reg_addr, &value, 1);
}

/**
 * @brief Read 16-bit value (big-endian)
 */
static iqs_error_e iqs_read_word(uint16_t reg_addr, uint16_t *value)
{
    uint8_t buf[2];
    iqs_error_e err = iqs_read_register(reg_addr, buf, 2);
    if (err == IQS_OK)
    {
        *value = (buf[0] << 8) | buf[1];  // Big-endian
    }
    return err;
}

/**
 * @brief Write 16-bit value (big-endian)
 */
static iqs_error_e iqs_write_word(uint16_t reg_addr, uint16_t value)
{
    uint8_t buf[2];
    buf[0] = (value >> 8) & 0xFF;  // High byte first (big-endian)
    buf[1] = value & 0xFF;         // Low byte
    return iqs_write_register(reg_addr, buf, 2);
}

/*============================================================================
 * Get Default Configuration
 *===========================================================================*/
void iqs550_get_default_config(iqs550_config_t *config)
{
    if (config == NULL) return;

    memset(config, 0, sizeof(iqs550_config_t));

    // Channel configuration for Tx(0-14), Rx(0-9)
    config->channels.total_rx = IQS550_TOTAL_RX_CHANNELS;  // 10
    config->channels.total_tx = IQS550_TOTAL_TX_CHANNELS;  // 15

    // Enable all Tx (0-14): bits 0-14 = 0x7FFF
    config->channels.tx_enable = 0x7FFF;

    // Enable all Rx (0-9): bits 0-9 = 0x03FF
    config->channels.rx_enable = 0x03FF;

    // Default 1:1 mapping for Rx
    for (uint8_t i = 0; i < 10; i++)
    {
        config->channels.rx_mapping[i] = i;
    }

    // Default 1:1 mapping for Tx
    for (uint8_t i = 0; i < 15; i++)
    {
        config->channels.tx_mapping[i] = i;
    }

    // Resolution: (channels - 1) * 256
    config->channels.x_resolution = IQS550_X_RESOLUTION;  // 3584
    config->channels.y_resolution = IQS550_Y_RESOLUTION;  // 2304

    // XY config: no flip, no swap (adjust based on your PCB orientation)
    config->channels.xy_config = 0x00;

    // ATI configuration (default values - may need tuning)
    config->ati.ati_target = 500;       // Typical target value
    config->ati.ati_c = 0;              // Auto-compensation
    config->ati.prox_threshold = 10;    // Proximity threshold
    config->ati.touch_threshold = 6;    // Touch threshold multiplier
    config->ati.snap_threshold = 50;    // Snap threshold

    // System configuration
    config->event_mode = true;          // Use event mode (RDY pin asserts on touch)
    config->wdt_enable = true;          // Enable watchdog
}

/*============================================================================
 * Device Identification
 *===========================================================================*/
iqs_error_e iqs550_read_device_id(iqs550_dev_id_t *dev_id)
{
    if (dev_id == NULL)
    {
        return IQS_ERR_INVALID_PARAM;
    }

    uint8_t buf[8];
    iqs_error_e err;

    // Read product number (2 bytes at 0x0000)
    err = iqs_read_register(IQS550_REG_PROD_NUM, buf, 7);
    if (err != IQS_OK)
    {
        return err;
    }

    dev_id->prod_num = (buf[0] << 8) | buf[1];
    dev_id->proj_num = (buf[2] << 8) | buf[3];
    dev_id->major_ver = buf[4];
    dev_id->minor_ver = buf[5];
    dev_id->bl_status = buf[6];

    LOG_INFO("Device ID: Product=0x%04X, Project=0x%04X, Version=%d.%d",
             dev_id->prod_num, dev_id->proj_num,
             dev_id->major_ver, dev_id->minor_ver);

    return IQS_OK;
}

/*============================================================================
 * Channel Configuration
 *===========================================================================*/
iqs_error_e iqs550_configure_channels(const iqs550_channel_config_t *ch_config)
{
    if (ch_config == NULL)
    {
        return IQS_ERR_INVALID_PARAM;
    }

    iqs_error_e err;
    uint8_t buf[16];

    LOG_INFO("Configuring channels: Tx=%d, Rx=%d",
             ch_config->total_tx, ch_config->total_rx);

    // Step 1: Set total Rx and Tx counts
    buf[0] = ch_config->total_rx;
    buf[1] = ch_config->total_tx;
    err = iqs_write_register(IQS550_REG_TOTAL_RX, buf, 2);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set Rx/Tx counts");
        return err;
    }

    // Step 2: Set Rx enable mask (16 bits, little-endian in register)
    buf[0] = ch_config->rx_enable & 0xFF;         // Low byte (Rx0-7)
    buf[1] = (ch_config->rx_enable >> 8) & 0xFF;  // High byte (Rx8-9)
    err = iqs_write_register(IQS550_REG_RX_ENABLE_L, buf, 2);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set Rx enable mask");
        return err;
    }
    LOG_DBG("Rx enable mask: 0x%04X", ch_config->rx_enable);

    // Step 3: Set Tx enable mask (16 bits)
    buf[0] = ch_config->tx_enable & 0xFF;         // Low byte (Tx0-7)
    buf[1] = (ch_config->tx_enable >> 8) & 0xFF;  // High byte (Tx8-14)
    err = iqs_write_register(IQS550_REG_TX_ENABLE_L, buf, 2);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set Tx enable mask");
        return err;
    }
    LOG_DBG("Tx enable mask: 0x%04X", ch_config->tx_enable);

    // Step 4: Set Rx mapping (10 bytes for your config)
    err = iqs_write_register(IQS550_REG_RX_MAPPING,
                            (uint8_t *)ch_config->rx_mapping,
                            ch_config->total_rx);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set Rx mapping");
        return err;
    }

    // Step 5: Set Tx mapping (15 bytes for your config)
    err = iqs_write_register(IQS550_REG_TX_MAPPING,
                            (uint8_t *)ch_config->tx_mapping,
                            ch_config->total_tx);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set Tx mapping");
        return err;
    }

    // Step 6: Set resolution
    err = iqs_write_word(IQS550_REG_X_RES, ch_config->x_resolution);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set X resolution");
        return err;
    }

    err = iqs_write_word(IQS550_REG_Y_RES, ch_config->y_resolution);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set Y resolution");
        return err;
    }
    LOG_DBG("Resolution: %dx%d", ch_config->x_resolution, ch_config->y_resolution);

    // Step 7: Set XY configuration (axis flip/swap)
    err = iqs_write_byte(IQS550_REG_XY_CONFIG, ch_config->xy_config);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set XY config");
        return err;
    }

    return IQS_OK;
}

/*============================================================================
 * Individual Tx/Rx Enable/Disable (based on your logic requirement)
 *===========================================================================*/
iqs_error_e iqs550_set_tx_enable(uint8_t tx_num, bool enable)
{
    if (tx_num >= IQS550_TOTAL_TX_CHANNELS)
    {
        LOG_ERR("Invalid Tx number: %d (max %d)", tx_num, IQS550_TOTAL_TX_CHANNELS - 1);
        return IQS_ERR_INVALID_PARAM;
    }

    uint8_t buf[2];
    iqs_error_e err;

    // Read current Tx enable mask
    err = iqs_read_register(IQS550_REG_TX_ENABLE_L, buf, 2);
    if (err != IQS_OK)
    {
        return err;
    }

    uint16_t tx_mask = buf[0] | (buf[1] << 8);

    // Modify the bit
    if (enable)
    {
        tx_mask |= (1 << tx_num);
    }
    else
    {
        tx_mask &= ~(1 << tx_num);
    }

    // Write back
    buf[0] = tx_mask & 0xFF;
    buf[1] = (tx_mask >> 8) & 0xFF;
    err = iqs_write_register(IQS550_REG_TX_ENABLE_L, buf, 2);

    LOG_DBG("Tx%d %s, mask=0x%04X", tx_num, enable ? "enabled" : "disabled", tx_mask);

    return err;
}

iqs_error_e iqs550_set_rx_enable(uint8_t rx_num, bool enable)
{
    if (rx_num >= IQS550_TOTAL_RX_CHANNELS)
    {
        LOG_ERR("Invalid Rx number: %d (max %d)", rx_num, IQS550_TOTAL_RX_CHANNELS - 1);
        return IQS_ERR_INVALID_PARAM;
    }

    uint8_t buf[2];
    iqs_error_e err;

    // Read current Rx enable mask
    err = iqs_read_register(IQS550_REG_RX_ENABLE_L, buf, 2);
    if (err != IQS_OK)
    {
        return err;
    }

    uint16_t rx_mask = buf[0] | (buf[1] << 8);

    // Modify the bit
    if (enable)
    {
        rx_mask |= (1 << rx_num);
    }
    else
    {
        rx_mask &= ~(1 << rx_num);
    }

    // Write back
    buf[0] = rx_mask & 0xFF;
    buf[1] = (rx_mask >> 8) & 0xFF;
    err = iqs_write_register(IQS550_REG_RX_ENABLE_L, buf, 2);

    LOG_DBG("Rx%d %s, mask=0x%04X", rx_num, enable ? "enabled" : "disabled", rx_mask);

    return err;
}

/*============================================================================
 * ATI (Auto-Tuning Implementation)
 *===========================================================================*/
static iqs_error_e iqs550_configure_ati(const iqs550_ati_config_t *ati_config)
{
    if (ati_config == NULL)
    {
        return IQS_ERR_INVALID_PARAM;
    }

    iqs_error_e err;

    // Set ATI target
    err = iqs_write_word(IQS550_REG_ATI_TARGET, ati_config->ati_target);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set ATI target");
        return err;
    }

    // Set thresholds
    err = iqs_write_byte(IQS550_REG_PROX_THRESH, ati_config->prox_threshold);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set proximity threshold");
        return err;
    }

    err = iqs_write_byte(IQS550_REG_TOUCH_THRESH, ati_config->touch_threshold);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set touch threshold");
        return err;
    }

    err = iqs_write_byte(IQS550_REG_SNAP_THRESH, ati_config->snap_threshold);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set snap threshold");
        return err;
    }

    LOG_INFO("ATI configured: target=%d, prox=%d, touch=%d, snap=%d",
             ati_config->ati_target, ati_config->prox_threshold,
             ati_config->touch_threshold, ati_config->snap_threshold);

    return IQS_OK;
}

iqs_error_e iqs550_trigger_ati(void)
{
    // Read current SYS_CTRL1
    uint8_t ctrl1;
    iqs_error_e err = iqs_read_byte(IQS550_REG_SYS_CTRL1, &ctrl1);
    if (err != IQS_OK)
    {
        return err;
    }

    // Set Re-ATI bit
    ctrl1 |= IQS550_SYS_CTRL1_REATI;
    err = iqs_write_byte(IQS550_REG_SYS_CTRL1, ctrl1);

    LOG_INFO("ATI triggered");
    return err;
}

iqs_error_e iqs550_wait_ati_complete(uint32_t timeout_ms)
{
    uint32_t elapsed = 0;
    const uint32_t poll_interval = 10;  // ms

    while (elapsed < timeout_ms)
    {
        uint8_t sys_info0;
        iqs_error_e err = iqs_read_byte(IQS550_REG_SYS_INFO0, &sys_info0);
        if (err != IQS_OK)
        {
            return err;
        }

        // Check for ATI error
        if (sys_info0 & IQS550_SYS_INFO0_ATI_ERROR)
        {
            LOG_ERR("ATI error detected");
            return IQS_ERR_ATI_FAILED;
        }

        // ATI complete when REATI bit clears in SYS_CTRL1
        uint8_t ctrl1;
        err = iqs_read_byte(IQS550_REG_SYS_CTRL1, &ctrl1);
        if (err != IQS_OK)
        {
            return err;
        }

        if (!(ctrl1 & IQS550_SYS_CTRL1_REATI))
        {
            LOG_INFO("ATI completed successfully");
            return IQS_OK;
        }

        delay_ms(poll_interval);
        elapsed += poll_interval;
    }

    LOG_ERR("ATI timeout after %lu ms", (unsigned long)timeout_ms);
    return IQS_ERR_TIMEOUT;
}

/*============================================================================
 * System Control Functions
 *===========================================================================*/
iqs_error_e iqs550_ack_reset(void)
{
    return iqs_write_byte(IQS550_REG_SYS_CTRL0, IQS550_SYS_CTRL0_ACK_RESET);
}

iqs_error_e iqs550_sw_reset(void)
{
    return iqs_write_byte(IQS550_REG_SYS_CTRL0, IQS550_SYS_CTRL0_SW_RESET);
}

iqs_error_e iqs550_suspend(void)
{
    return iqs_write_byte(IQS550_REG_SYS_CTRL0, IQS550_SYS_CTRL0_SUSPEND);
}

iqs_error_e iqs550_resume(void)
{
    return iqs_write_byte(IQS550_REG_SYS_CTRL0, 0x00);
}

bool iqs550_is_ready(void)
{
    // Option 1: Check RDY GPIO pin (implement based on your hardware)
    // return (GPIO_PinInGet(IQS_RDY_PORT, IQS_RDY_PIN) == 0);

    // Option 2: Try a simple I2C read
    uint8_t dummy;
    return (iqs_read_byte(IQS550_REG_PROD_NUM, &dummy) == IQS_OK);
}

iqs_error_e iqs550_wait_ready(uint32_t timeout_ms)
{
    uint32_t elapsed = 0;
    const uint32_t poll_interval = 5;  // ms

    while (elapsed < timeout_ms)
    {
        if (iqs550_is_ready())
        {
            return IQS_OK;
        }
        delay_ms(poll_interval);
        elapsed += poll_interval;
    }

    return IQS_ERR_TIMEOUT;
}

/*============================================================================
 * Status and Touch Data Reading
 *===========================================================================*/
iqs_error_e iqs550_read_status(iqs550_status_t *status)
{
    if (status == NULL)
    {
        return IQS_ERR_INVALID_PARAM;
    }

    uint8_t buf[3];
    iqs_error_e err;

    // Read SYS_INFO0 and gesture registers
    err = iqs_read_register(IQS550_REG_GESTURE_EVENTS0, buf, 3);
    if (err != IQS_OK)
    {
        return err;
    }

    status->gesture_single = buf[0];
    status->gesture_multi = buf[1];

    uint8_t sys_info0 = buf[2];
    status->reset_occurred = (sys_info0 & IQS550_SYS_INFO0_SHOW_RESET) != 0;
    status->ati_error = (sys_info0 & IQS550_SYS_INFO0_ATI_ERROR) != 0;
    status->alp_ati_error = (sys_info0 & IQS550_SYS_INFO0_ALP_ATI_ERROR) != 0;
    status->gesture_event = (sys_info0 & IQS550_SYS_INFO0_GESTURE) != 0;
    status->tp_movement = (sys_info0 & IQS550_SYS_INFO0_TP_MOVEMENT) != 0;
    status->palm_detected = (sys_info0 & IQS550_SYS_INFO0_PALM_DETECT) != 0;

    return IQS_OK;
}

iqs_error_e iqs550_read_touch_data(iqs550_touch_data_t *touch_data)
{
    if (touch_data == NULL)
    {
        return IQS_ERR_INVALID_PARAM;
    }

    memset(touch_data, 0, sizeof(iqs550_touch_data_t));

    uint8_t buf[44];  // Read all touch data at once
    iqs_error_e err;

    // Read from NUM_FINGERS through all 5 finger positions
    err = iqs_read_register(IQS550_REG_NUM_FINGERS, buf, 44);
    if (err != IQS_OK)
    {
        return err;
    }

    touch_data->num_fingers = buf[0];
    touch_data->rel_x = (int16_t)((buf[1] << 8) | buf[2]);
    touch_data->rel_y = (int16_t)((buf[3] << 8) | buf[4]);

    // Parse finger data (each finger: X(2) + Y(2) + strength(2) + area(1) = 7 bytes + padding)
    // Layout varies - adjust based on actual register map
    if (touch_data->num_fingers > 0)
    {
        // Finger 1 starts at offset 5
        touch_data->points[0].abs_x = (buf[5] << 8) | buf[6];
        touch_data->points[0].abs_y = (buf[7] << 8) | buf[8];
        touch_data->points[0].touch_strength = (buf[9] << 8) | buf[10];
        touch_data->points[0].touch_area = buf[11];
    }

    // Continue for additional fingers if present
    // (Adjust offsets based on actual IQS550 register layout)

    return IQS_OK;
}

/*============================================================================
 * Raw Channel Data Reading
 *===========================================================================*/
iqs_error_e iqs550_read_channel_count(uint8_t channel, uint16_t *count)
{
    if (channel >= IQS550_TOTAL_CHANNELS || count == NULL)
    {
        return IQS_ERR_INVALID_PARAM;
    }

    // Calculate address: base + (channel * 2)
    uint16_t addr = IQS550_REG_COUNT_DATA + (channel * 2);
    return iqs_read_word(addr, count);
}

iqs_error_e iqs550_read_channel_delta(uint8_t channel, int16_t *delta)
{
    if (channel >= IQS550_TOTAL_CHANNELS || delta == NULL)
    {
        return IQS_ERR_INVALID_PARAM;
    }

    // Calculate address: base + (channel * 2)
    uint16_t addr = IQS550_REG_DELTA_DATA + (channel * 2);
    return iqs_read_word(addr, (uint16_t *)delta);
}

/*============================================================================
 * Main Initialization Function
 *===========================================================================*/
iqs_error_e iqs550_init(const iqs550_config_t *config)
{
    iqs_error_e err;
    iqs550_config_t default_config;
    const iqs550_config_t *cfg;

    LOG_INFO("=== IQS550 Initialization Started ===");

    // Use provided config or get defaults
    if (config == NULL)
    {
        iqs550_get_default_config(&default_config);
        cfg = &default_config;
        LOG_INFO("Using default configuration");
    }
    else
    {
        cfg = config;
        LOG_INFO("Using custom configuration");
    }

    /*--------------------------------------------------------------------
     * Step 1: Wait for device to be ready after power-up
     *------------------------------------------------------------------*/
    LOG_INFO("Step 1: Waiting for device ready...");
    err = iqs550_wait_ready(1000);  // 1 second timeout
    if (err != IQS_OK)
    {
        LOG_ERR("Device not responding");
        return IQS_ERR_NOT_READY;
    }

    /*--------------------------------------------------------------------
     * Step 2: Read and verify device ID
     *------------------------------------------------------------------*/
    LOG_INFO("Step 2: Verifying device ID...");
    iqs550_dev_id_t dev_id;
    err = iqs550_read_device_id(&dev_id);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to read device ID");
        return err;
    }

    // Verify it's an IQS550
    if (dev_id.prod_num != IQS550_PROD_NUM)
    {
        LOG_ERR("Invalid product number: expected %d, got %d",
                IQS550_PROD_NUM, dev_id.prod_num);
        return IQS_ERR_INVALID_DEVICE;
    }

    // Verify B000 firmware
    if (dev_id.proj_num != IQS550_PROJ_NUM_B000)
    {
        LOG_ERR("Expected B000 firmware (proj=%d), found proj=%d",
                IQS550_PROJ_NUM_B000, dev_id.proj_num);
        return IQS_ERR_WRONG_FIRMWARE;
    }

    // Check minimum version
    if (dev_id.major_ver < IQS550_MAJOR_VER_MIN)
    {
        LOG_ERR("Firmware version too old: %d.%d (min %d.x)",
                dev_id.major_ver, dev_id.minor_ver, IQS550_MAJOR_VER_MIN);
        return IQS_ERR_WRONG_FIRMWARE;
    }

    LOG_INFO("Device verified: IQS550-B000 v%d.%d",
             dev_id.major_ver, dev_id.minor_ver);

    /*--------------------------------------------------------------------
     * Step 3: Acknowledge any pending reset
     *------------------------------------------------------------------*/
    LOG_INFO("Step 3: Acknowledging reset...");
    uint8_t sys_info0;
    err = iqs_read_byte(IQS550_REG_SYS_INFO0, &sys_info0);
    if (err == IQS_OK && (sys_info0 & IQS550_SYS_INFO0_SHOW_RESET))
    {
        err = iqs550_ack_reset();
        if (err != IQS_OK)
        {
            LOG_ERR("Failed to acknowledge reset");
            return err;
        }
        LOG_INFO("Reset acknowledged");
    }

    /*--------------------------------------------------------------------
     * Step 4: Configure Tx/Rx channels
     *------------------------------------------------------------------*/
    LOG_INFO("Step 4: Configuring Tx/Rx channels...");
    LOG_INFO("  Tx channels: 0-14 (15 total)");
    LOG_INFO("  Rx channels: 0-9 (10 total)");

    err = iqs550_configure_channels(&cfg->channels);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to configure channels");
        return err;
    }

    /*--------------------------------------------------------------------
     * Step 5: Configure ATI parameters
     *------------------------------------------------------------------*/
    LOG_INFO("Step 5: Configuring ATI parameters...");
    err = iqs550_configure_ati(&cfg->ati);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to configure ATI");
        return err;
    }

    /*--------------------------------------------------------------------
     * Step 6: Configure system settings
     *------------------------------------------------------------------*/
    LOG_INFO("Step 6: Configuring system settings...");
    uint8_t sys_cfg0 = IQS550_SYS_CFG0_SETUP_COMPLETE |  // Mark setup complete
                       IQS550_SYS_CFG0_REATI |           // Enable re-ATI
                       IQS550_SYS_CFG0_ALP_REATI;        // Enable ALP re-ATI

    if (cfg->wdt_enable)
    {
        sys_cfg0 |= IQS550_SYS_CFG0_WDT_ENABLE;
    }

    if (cfg->event_mode)
    {
        sys_cfg0 |= IQS550_SYS_CFG0_EVENT_MODE;
    }

    err = iqs_write_byte(IQS550_REG_SYS_CFG0, sys_cfg0);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to write SYS_CFG0");
        return err;
    }

    // Configure event mode details in SYS_CFG1
    if (cfg->event_mode)
    {
        uint8_t sys_cfg1 = IQS550_SYS_CFG1_TP_EVENT;  // Trackpad events
        err = iqs_write_byte(IQS550_REG_SYS_CFG1, sys_cfg1);
        if (err != IQS_OK)
        {
            LOG_ERR("Failed to write SYS_CFG1");
            return err;
        }
    }

    /*--------------------------------------------------------------------
     * Step 7: Trigger ATI and wait for completion
     *------------------------------------------------------------------*/
    LOG_INFO("Step 7: Running Auto-Tuning Implementation (ATI)...");
    err = iqs550_trigger_ati();
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to trigger ATI");
        return err;
    }

    err = iqs550_wait_ati_complete(5000);  // 5 second timeout for ATI
    if (err != IQS_OK)
    {
        LOG_ERR("ATI failed");
        return err;
    }

    /*--------------------------------------------------------------------
     * Step 8: Final verification
     *------------------------------------------------------------------*/
    LOG_INFO("Step 8: Final verification...");
    err = iqs_read_byte(IQS550_REG_SYS_INFO0, &sys_info0);
    if (err != IQS_OK)
    {
        return err;
    }

    if (sys_info0 & IQS550_SYS_INFO0_ATI_ERROR)
    {
        LOG_ERR("ATI error flag set after initialization");
        return IQS_ERR_ATI_FAILED;
    }

    LOG_INFO("=== IQS550 Initialization Complete ===");
    LOG_INFO("Ready for touch detection on %d channels (Tx0-14 x Rx0-9)",
             IQS550_TOTAL_CHANNELS);

    return IQS_OK;
}

/*============================================================================
 * Example: Bulk Tx/Rx Enable/Disable
 *===========================================================================*/

/**
 * @brief Enable/disable multiple Tx channels at once
 * @param tx_mask Bitmask of Tx channels (bit 0 = Tx0, bit 14 = Tx14)
 * @param enable true to enable selected channels, false to disable
 */
iqs_error_e iqs550_set_tx_enable_mask(uint16_t tx_mask, bool enable)
{
    uint8_t buf[2];
    iqs_error_e err;

    // Read current mask
    err = iqs_read_register(IQS550_REG_TX_ENABLE_L, buf, 2);
    if (err != IQS_OK)
    {
        return err;
    }

    uint16_t current_mask = buf[0] | (buf[1] << 8);

    if (enable)
    {
        current_mask |= tx_mask;
    }
    else
    {
        current_mask &= ~tx_mask;
    }

    // Limit to valid Tx range (0-14)
    current_mask &= 0x7FFF;

    buf[0] = current_mask & 0xFF;
    buf[1] = (current_mask >> 8) & 0xFF;

    return iqs_write_register(IQS550_REG_TX_ENABLE_L, buf, 2);
}

/**
 * @brief Enable/disable multiple Rx channels at once
 * @param rx_mask Bitmask of Rx channels (bit 0 = Rx0, bit 9 = Rx9)
 * @param enable true to enable selected channels, false to disable
 */
iqs_error_e iqs550_set_rx_enable_mask(uint16_t rx_mask, bool enable)
{
    uint8_t buf[2];
    iqs_error_e err;

    // Read current mask
    err = iqs_read_register(IQS550_REG_RX_ENABLE_L, buf, 2);
    if (err != IQS_OK)
    {
        return err;
    }

    uint16_t current_mask = buf[0] | (buf[1] << 8);

    if (enable)
    {
        current_mask |= rx_mask;
    }
    else
    {
        current_mask &= ~rx_mask;
    }

    // Limit to valid Rx range (0-9)
    current_mask &= 0x03FF;

    buf[0] = current_mask & 0xFF;
    buf[1] = (current_mask >> 8) & 0xFF;

    return iqs_write_register(IQS550_REG_RX_ENABLE_L, buf, 2);
}
