/*
 * iqs572.c
 *
 *  Created on: Oct 30, 2025
 *      Author: Bhakti Ramani
 */

#include "../inc/iqs572.h"

#define IQS_READ_VERSION 0x0000
#define IQS_READ_X_CORD 0x012
#define IQS_READ_Y_COORD 0x014

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
        default:                         return "IQS_ERR_UNKNOWN";
    }
}

//static iqs_error_e iqs_read_register(uint16_t reg_addr, uint8_t *data, size_t len)
//{
//    if (data == NULL || len == 0)
//    {
//        LOG_ERR("Invalid parameters: data=%p, len=%zu", data, len);
//        return IQS_ERR_INVALID_PARAM;
//    }
//
//    I2C_TransferReturn_TypeDef status = i2c_read(IQS_I2C_ADDR, reg_addr, data, len);
//
//    if (status != i2cTransferDone)
//    {
//        LOG_ERR("I2C read failed at address 0x%04X, status=%d", reg_addr, status);
//        return IQS_ERR_I2C_READ_FAIL;
//    }
//
////    LOG_INFO("Successfully read %d bytes from 0x%04X", len, reg_addr);
//    return IQS_OK;
//}
//
//static iqs_error_e iqs_write_register(uint16_t reg_addr, uint8_t *data, size_t len)
//{
//    if (data == NULL || len == 0)
//    {
//        LOG_ERR("Invalid parameters: data=%p, len=%d", data, len);
//        return IQS_ERR_INVALID_PARAM;
//    }
//
//    I2C_TransferReturn_TypeDef status = i2c_write(IQS_I2C_ADDR, reg_addr, data, len);
//
//    if (status != i2cTransferDone)
//    {
//        LOG_ERR("I2C write failed at address 0x%04X, status=%d", reg_addr, status);
//        return IQS_ERR_I2C_WRITE_FAIL;
//    }
//
////    LOG_INFO("Successfully wrote %d bytes to 0x%04X", len, reg_addr);
//    return IQS_OK;
//}



iqs_error_e iqs_read_register(uint16_t reg_addr, uint8_t *data, size_t len)
{
    if (data == NULL || len == 0)
    {
        LOG_ERR("Invalid parameters: data=%p, len=%zu", data, len);
        return IQS_ERR_INVALID_PARAM;
    }

    I2C_TransferReturn_TypeDef status;

    for (int attempt = 1; attempt <= IQS_I2C_RETRY_COUNT; attempt++)
    {
        status = i2c_read(IQS_I2C_ADDR, reg_addr, data, len);

        if (status == i2cTransferDone)
        {
            if (attempt > 1)
            {
//                LOG_INFO("I2C read succeeded on attempt %d (addr 0x%04X)", attempt, reg_addr);
            }
            return IQS_OK;
        }

        if (attempt < IQS_I2C_RETRY_COUNT)
        {
//            LOG_WARN("I2C read failed at 0x%04X (attempt %d/%d), retrying...",
//                     reg_addr, attempt, IQS_I2C_RETRY_COUNT);
            sl_udelay_wait(IQS_I2C_RETRY_DELAY_US);
        }
    }

    LOG_ERR("I2C read failed at 0x%04X after %d attempts", reg_addr, IQS_I2C_RETRY_COUNT);
    return IQS_ERR_I2C_READ_FAIL;
}

iqs_error_e iqs_write_register(uint16_t reg_addr, uint8_t *data, size_t len)
{
    if (data == NULL || len == 0)
    {
        LOG_ERR("Invalid parameters: data=%p, len=%zu", data, len);
        return IQS_ERR_INVALID_PARAM;
    }

    I2C_TransferReturn_TypeDef status;

    for (int attempt = 1; attempt <= IQS_I2C_RETRY_COUNT; attempt++)
    {
        status = i2c_write(IQS_I2C_ADDR, reg_addr, data, len);

        if (status == i2cTransferDone)
        {
            if (attempt > 1)
            {
//                LOG_INFO("I2C write succeeded on attempt %d (addr 0x%04X)", attempt, reg_addr);
            }
            return IQS_OK;
        }

        if (attempt < IQS_I2C_RETRY_COUNT)
        {
//            LOG_WARN("I2C write failed at 0x%04X (attempt %d/%d), retrying...",
//                     reg_addr, attempt, IQS_I2C_RETRY_COUNT);
            sl_udelay_wait(IQS_I2C_RETRY_DELAY_US);
        }
    }

    LOG_ERR("I2C write failed at 0x%04X after %d attempts", reg_addr, IQS_I2C_RETRY_COUNT);
    return IQS_ERR_I2C_WRITE_FAIL;
}
iqs_error_e iqs_ack_reset(void)
{
    iqs_error_e err;
    uint8_t reset_cmd = 0x80;  // ACK_RESET bit

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



iqs_error_e iqs_check_setup_window(void){
  iqs_error_e err;

  uint8_t setup_complete = 0;
  err = iqs_write_register(IQS_REG_SYS_CONFIG_0, &setup_complete, 1);
  if (err != IQS_OK)
  {
      LOG_ERR("Failed to read system setup complete : %s", iqs_error_to_str(err));
      return err;
  }

  if((setup_complete & IQS_SYS_CONFIG_0_SETUP_COMPLETE_PIN) != 0){
      LOG_ERR("System is not in setup mode. Setup Complete is 1");
  }
  LOG_INFO("System is in setup mode. Setup Complete is 0");
  return IQS_OK;
}
iqs_error_e iqs_reset(void)
{
    iqs_error_e err;
    uint8_t reset_cmd = IQS_SYS_CONTROL_1_RESET_PIN;

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

    LOG_INFO("System flags: 0x%02X 0x%02X", sys_flags[0], sys_flags[1]);
    return IQS_OK;
}


///*============================================================================
// * Auto ATI with Retry on Failure
// *===========================================================================*/
//iqs_error_e iqs_auto_ATI_with_retry(uint8_t max_retries)
//{
//    iqs_error_e err;
//    uint16_t ati_targets[] = {500, 400, 600, 300, 800};  // Different targets to try
//    uint8_t num_targets = sizeof(ati_targets) / sizeof(ati_targets[0]);
//
//    LOG_INFO("=== Auto ATI with Retry (max %d attempts) ===", max_retries);
//
//    for (uint8_t attempt = 0; attempt < max_retries; attempt++)
//    {
//        uint16_t target = ati_targets[attempt % num_targets];
//
//        LOG_INFO("Attempt %d/%d with ATI target = %d", attempt + 1, max_retries, target);
//
//        // Set ATI target
//        err = iqs_set_ati_target(target);
//        if (err != IQS_OK)
//        {
//            continue;
//        }
//
//        // Set same target for ALP
//        err = iqs_set_alp_ati_target(target);
//        if (err != IQS_OK)
//        {
//            continue;
//        }
//
//        // Set relaxed Re-ATI limits for higher attempts
//        uint8_t lower = (attempt < 2) ? 5 : 2;
//        uint8_t upper = (attempt < 2) ? 50 : 100;
//        iqs_set_reati_limits(lower, upper);
//
//        // Try Auto ATI
//        err = iqs_auto_ATI();
//        if (err == IQS_OK)
//        {
//            LOG_INFO("ATI succeeded on attempt %d with target %d", attempt + 1, target);
//            return IQS_OK;
//        }
//
//        LOG_WARN("ATI attempt %d failed, retrying...", attempt + 1);
//
//        // Delay before retry
//        sl_udelay_wait(50000);  // 50ms delay
//    }
//
//    LOG_ERR("ATI failed after %d attempts!", max_retries);
//    return IQS_ERR_NOT_INITIALIZED;
//}

iqs_error_e iqs_auto_ATI(void){
  iqs_error_e err;
  uint8_t buf[2];
  LOG_INFO("Configuring AUTO ATI and REATI ...");

  // set ATI_TARGET (16-bit)
  uint16_t ati_target = IQS_ATI_TARGET;
//  buf[0] = (ati_target >> 8) & 0xFF;   // MSB first
//  buf[1] = ati_target & 0xFF;          // LSB
//  buf[0] = 0xE8;  // Low byte first
//  buf[1] = 0x03;  // High byte = 0x0200 = 512
//
//  LOG_INFO("  Writing ATI_TARGET: 0x%02X 0x%02X", buf[0], buf[1]);
//  err = iqs_write_register(IQS_REG_ATI_TARGET, buf, 2);
//  if (err != IQS_OK) {
//      LOG_ERR("Failed to write ATI_TARGET: %s", iqs_error_to_str(err));
//      return err;
//  }

  // Step 1: Enter Setup Mode
  LOG_INFO("Entering Setup Mode...");
  iqs_read_register(0x0432, buf, 2);  // SYSTEM_CONTROL_1
  buf[0] &= ~0x02;  // Clear SETUP_COMPLETE bit (bit 1) to enter setup mode
  iqs_write_register(0x0432, buf, 2);
  sl_sleeptimer_delay_millisecond(50);

  // Verify we're in setup mode
  iqs_read_register(0x058E, buf, 2);  // SYSTEM_INFO_0
  LOG_INFO("SYSTEM_INFO_0: 0x%02X (SETUP_COMPLETE should be 0)", buf[0]);
  if (buf[0] & 0x40) {
      LOG_ERR("Failed to enter setup mode!");
      return err;
  }

  // Set ATI_TARGET to 500 (0x01F4)
  uint8_t target[2] = {0x01, 0xF4};  // 500 in big-endian
  iqs_write_register(0x058E, target, 2);

  // Readback
  err = iqs_read_register(IQS_REG_ATI_TARGET, buf, 2);
  if (err != IQS_OK) {
      LOG_ERR("Failed to read ATI_TARGET: %s", iqs_error_to_str(err));
      return err;
  }

  uint16_t readback_le = buf[0] | (buf[1] << 8);  // Little endian interpretation
  uint16_t readback_be = (buf[0] << 8) | buf[1];  // Big endian interpretation
  LOG_INFO("  ATI_TARGET readback raw: 0x%02X 0x%02X", buf[0], buf[1]);
  LOG_INFO("  ATI_TARGET (LE): %d (BE) : %d", readback_le,readback_be);


  // REATI and ALPREATI set in system config
//  uint8_t sys_config_flag = IQS_SYS_CONFIG_0_REATI | IQS_SYS_CONFIG_0_ALP_REATI;
//  err = iqs_write_register(IQS_REG_SYS_CONFIG_0, &sys_config_flag, 1);
//  if (err != IQS_OK)
//  {
//      LOG_ERR("Failed to write system config 0 REATI | APL_REATI bit: %s", iqs_error_to_str(err));
//      return err;
//  }

  // set auto_ATI bit in system control 0 register
  uint8_t sys_control_flag = IQS_SYS_CONTROL_0_AUTO_ATI;
  err = iqs_write_register(IQS_REG_SYS_CONTROL_0, &sys_control_flag, 1);
  if (err != IQS_OK)
  {
      LOG_ERR("Failed to set AUTO_ATI bit: %s", iqs_error_to_str(err));
      return err;
  }


  //wait for auto_ATI event to be over
  uint8_t ati_status = 0;
  err = iqs_read_register(IQS_REG_SYS_CONTROL_0, &ati_status, 1);
  if (err != IQS_OK)
  {
      LOG_ERR("Failed to read AUTO_ATI bit: %s", iqs_error_to_str(err));
      return err;
  }
  while(ati_status & IQS_SYS_CONTROL_0_AUTO_ATI){
      sl_udelay_wait(500);  // ms
      err = iqs_read_register(IQS_REG_SYS_CONTROL_0, &ati_status, 1);
      if (err != IQS_OK)
      {
          LOG_ERR("Failed to read AUTO_ATI bit: %s", iqs_error_to_str(err));
          return err;
      }
  }

  // check for ATI error
  uint8_t sys_info = 0;
  err = iqs_read_register(IQS_REG_SYSTEM_INFO_0, &sys_info, 1);
  if (err != IQS_OK)
  {
      LOG_ERR("Failed to read AUTO_ATI bit: %s", iqs_error_to_str(err));
      return err;
  }
  if((sys_info & IQS_SYS_INFO_0_ATI_ERROR) || (sys_info & IQS_SYS_INFO_0_ALP_ATI_ERROR)){
      LOG_ERR("AUTO ATI Failed, system info 0 :  0x%02X", sys_info);
//      return err;
  }

  uint8_t sys_info1 = 0;
  err = iqs_read_register(IQS_REG_SYSTEM_INFO_1, &sys_info1, 1);
  if (err != IQS_OK)
  {
      LOG_ERR("Failed to read AUTO_ATI bit: %s", iqs_error_to_str(err));
      return err;
  }
  LOG_ERR("system info 1 :  0x%02X", sys_info1);
  return IQS_OK;
}

iqs_error_e iqs_sys_config(void){
  iqs_error_e err;

  LOG_INFO("Configuring system (Event Mode + PROX + Gesture)...");
//  uint8_t sys_config = IQS_SYS_CONFIG_PROX_PIN | IQS_SYS_CONFIG_EVENT_MODE_PIN | IQS_SYS_CONFIG_GES_PIN;

  uint8_t current_val = 0x00;

  err = iqs_read_register(IQS_REG_SYS_CONFIG_1, &current_val, 1);
  if (err != IQS_OK)
  {
      LOG_ERR("Failed to read back system config: %s", iqs_error_to_str(err));
      return err;
  }

  uint8_t sys_config = IQS_SYS_CONFIG_TP_PIN | IQS_SYS_CONFIG_EVENT_MODE_PIN | IQS_SYS_CONFIG_0_SETUP_COMPLETE_PIN;
//  uint8_t sys_config = IQS_SYS_CONFIG_0_SETUP_COMPLETE_PIN;
  current_val &= ~(0x01 | 0x04);  // Clear bit 0 and bit 2
  current_val |= sys_config;  // OR in 0x05


  err = iqs_write_register(IQS_REG_SYS_CONFIG_1, &sys_config, 1);
  if (err != IQS_OK)
  {
      LOG_ERR("Failed to write system config: %s", iqs_error_to_str(err));
      return err;
  }

  LOG_INFO("System config written: 0x%02X (Event Mode | PROX enabled | Gesture enabled)", sys_config);

  sl_udelay_wait(2000);  // 2ms

  LOG_INFO("Verifying system config...");
  uint8_t sys_config_readback = 0x00;

  err = iqs_read_register(IQS_REG_SYS_CONFIG_1, &sys_config_readback, 1);
  if (err != IQS_OK)
  {
      LOG_ERR("Failed to read back system config: %s", iqs_error_to_str(err));
      return err;
  }

  LOG_INFO("System config readback: 0x%02X", sys_config_readback);

//  if ((sys_config_readback & IQS_SYS_CONFIG_PROX_PIN) == 0)
//  {
//      LOG_WARN("PROX_PIN bit not set! Expected: 0x%02X, Got: 0x%02X",
//              sys_config, sys_config_readback);
////      return IQS_ERR_NOT_INITIALIZED;
//  }

  if ((sys_config_readback & IQS_SYS_CONFIG_EVENT_MODE_PIN) == 0)
  {
      LOG_ERR("EVENT_MODE bit not set! Device may not be in event mode");
      return IQS_ERR_NOT_INITIALIZED;
  }

//  if ((sys_config_readback & IQS_SYS_CONFIG_GES_PIN) == 0)
//  {
//      LOG_ERR("Gesture_PIN bit not set! Device may not be enabled for gestures");
//      return IQS_ERR_NOT_INITIALIZED;
//  }

  if ((sys_config_readback & IQS_SYS_CONFIG_TP_PIN) == 0)
  {
      LOG_ERR("IQS_SYS_CONFIG_TP_PIN bit not set! Device may not be enabled for gestures");
      return IQS_ERR_NOT_INITIALIZED;
  }

  LOG_INFO("Setup Compelete");

  uint8_t sys_config_0 = IQS_SYS_CONFIG_0_SETUP_COMPLETE_PIN;
  err = iqs_write_register(IQS_REG_SYS_CONFIG_0, &sys_config_0, 1);
  if (err != IQS_OK)
  {
      LOG_ERR("Failed to write setup complete: %s", iqs_error_to_str(err));
      return err;
  }

  LOG_INFO("Configuration verified successfully");
  return IQS_OK;
}

iqs_error_e iqs_gesture_enable(void){
  iqs_error_e err;

  uint8_t single_gesture_enable = IQS_SINGLE_FINGER_GESTURE_EN_MASK;
  err = iqs_write_register(IQS_REG_SINGLE_FINGER_GESTURE_EN, &single_gesture_enable, 1);
  if (err != IQS_OK)
  {
      LOG_ERR("Failed to write single tap gesture enable register: %s", iqs_error_to_str(err));
      return err;
  }

  uint8_t multi_gesture_enable = IQS_MULTI_FINGER_GESTURE_EN_MASK;
  err = iqs_write_register(IQS_REG_MULTI_FINGER_GESTURE_EN, &multi_gesture_enable, 1);
  if (err != IQS_OK)
  {
      LOG_ERR("Failed to write multi finger gesture enable register: %s", iqs_error_to_str(err));
      return err;
  }

  sl_udelay_wait(2000);  // 2ms

  uint8_t single_finger_gesture_enabled = 0x00;
  uint8_t multi_finger_gesture_enabled = 0x00;

  err = iqs_read_register(IQS_REG_SINGLE_FINGER_GESTURE_EN, &single_finger_gesture_enabled, 1);
  if (err != IQS_OK)
  {
      LOG_ERR("Failed to read back system config: %s", iqs_error_to_str(err));
      return err;
  }
  err = iqs_read_register(IQS_REG_MULTI_FINGER_GESTURE_EN, &multi_finger_gesture_enabled, 1);
  if (err != IQS_OK)
  {
      LOG_ERR("Failed to read back system config: %s", iqs_error_to_str(err));
      return err;
  }

  LOG_INFO("GESTURE EN registers : 0x%02X, 0x%02X", single_finger_gesture_enabled,multi_finger_gesture_enabled );

  if((single_finger_gesture_enabled & IQS_SINGLE_FINGER_GESTURE_EN_MASK) == 0){
      LOG_ERR("Failed to write single tap gesture enable register, error from readback: %s", iqs_error_to_str(err));
      return IQS_GESTURE_EN_FAIL;
  }
  if((multi_finger_gesture_enabled & IQS_MULTI_FINGER_GESTURE_EN_MASK) == 0){
      LOG_ERR("Failed to write multi tap gesture enable register, error from readback: %s", iqs_error_to_str(err));
      return IQS_GESTURE_EN_FAIL;
  }

  LOG_INFO("Successfully enabled Gestures");
  return IQS_OK;
}

iqs_error_e iqs_gesture_disable(void){
  iqs_error_e err;

  uint8_t single_gesture_enable = 0x00;
  err = iqs_write_register(IQS_REG_SINGLE_FINGER_GESTURE_EN, &single_gesture_enable, 1);
  if (err != IQS_OK)
  {
      LOG_ERR("Failed to write single tap gesture enable register: %s", iqs_error_to_str(err));
      return err;
  }

  uint8_t multi_gesture_enable = 0x00;
  err = iqs_write_register(IQS_REG_MULTI_FINGER_GESTURE_EN, &multi_gesture_enable, 1);
  if (err != IQS_OK)
  {
      LOG_ERR("Failed to write multi finger gesture enable register: %s", iqs_error_to_str(err));
      return err;
  }

  sl_udelay_wait(2000);  // 2ms

  uint8_t single_finger_gesture_enabled = 0x00;
  uint8_t multi_finger_gesture_enabled = 0x00;

  err = iqs_read_register(IQS_REG_SINGLE_FINGER_GESTURE_EN, &single_finger_gesture_enabled, 1);
  if (err != IQS_OK)
  {
      LOG_ERR("Failed to read back system config: %s", iqs_error_to_str(err));
      return err;
  }
  err = iqs_read_register(IQS_REG_MULTI_FINGER_GESTURE_EN, &multi_finger_gesture_enabled, 1);
  if (err != IQS_OK)
  {
      LOG_ERR("Failed to read back system config: %s", iqs_error_to_str(err));
      return err;
  }

  LOG_INFO("Disabled GESTURE EN registers : 0x%02X, 0x%02X", single_finger_gesture_enabled,multi_finger_gesture_enabled );

//  if((single_finger_gesture_enabled & IQS_SINGLE_FINGER_GESTURE_EN_MASK) == 0){
//      LOG_ERR("Failed to write single tap gesture enable register, error from readback: %s", iqs_error_to_str(err));
//      return IQS_GESTURE_EN_FAIL;
//  }
//  if((multi_finger_gesture_enabled & IQS_MULTI_FINGER_GESTURE_EN_MASK) == 0){
//      LOG_ERR("Failed to write multi tap gesture enable register, error from readback: %s", iqs_error_to_str(err));
//      return IQS_GESTURE_EN_FAIL;
//  }

  LOG_INFO("Successfully enabled Gestures");
  return IQS_OK;
}

iqs_error_e iqs_check_threshold(void){
  iqs_error_e err;
  uint8_t buf[2] = {0,0};

  err = iqs_read_register(IQS_REG_SNAP_THRESHOLD, &buf[0], 2);
  if (err != IQS_OK)
  {
      LOG_ERR("Failed to read Snap Threshold: %s", iqs_error_to_str(err));
      return err;
  }
  uint16_t readback = (buf[0] << 8 | buf[1]);
  LOG_INFO("Snap Threshold : 0x%04X (%d)", readback, readback);

  err = iqs_read_register(IQS_REG_PROX_THRESHOLD, &buf[0], 1);
  if (err != IQS_OK)
  {
      LOG_ERR("Failed to read Snap Threshold: %s", iqs_error_to_str(err));
      return err;
  }
  LOG_INFO("Snap Threshold : 0x%02X (%d)", buf[0], buf[0]);

  err = iqs_read_register(IQS_REG_ATI_TARGET, &buf[0], 2);
  if (err != IQS_OK)
  {
      LOG_ERR("Failed to read ATI_TARGET: %s", iqs_error_to_str(err));
      return err;
  }
  readback = (buf[0] << 8 | buf[1]);
  LOG_INFO("ATI_TARGET : 0x%04X (%d)", readback, readback);

  return IQS_OK;

}

iqs_error_e iqs_channel_config(void){
  iqs_error_e err;
  uint8_t buf[2];

  // set total_rx and total_tx
  uint8_t total_rx = IQS_RX_SIZE;
  err = iqs_write_register(IQS_REG_TOTAL_RX, &total_rx, 1);
  if (err != IQS_OK)
  {
      LOG_ERR("Failed to write total rx: %s", iqs_error_to_str(err));
      return err;
  }

  uint8_t total_tx = IQS_TX_SIZE;
  err = iqs_write_register(IQS_REG_TOTAL_TX, &total_tx, 1);
  if (err != IQS_OK)
  {
      LOG_ERR("Failed to write total tx: %s", iqs_error_to_str(err));
      return err;
  }

  // Verify Rx/Tx written correctly
  err = iqs_read_register(IQS_REG_TOTAL_RX, &total_rx, 1);
  LOG_INFO("  TOTAL_RX readback: %d", total_rx);
  err = iqs_read_register(IQS_REG_TOTAL_TX, &total_tx, 1);
  LOG_INFO("  TOTAL_TX readback: %d", total_tx);

  // set rx-tx mapping
  uint8_t rx_mapping[IQS_RX_SIZE] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  err = iqs_write_register(IQS_REG_RX_MAPPING, &rx_mapping[0], IQS_RX_SIZE);
  if (err != IQS_OK)
  {
      LOG_ERR("Failed to set Rx mapping");
      return err;
  }

  uint8_t tx_mapping[IQS_TX_SIZE] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,13, 14};
  err = iqs_write_register(IQS_REG_TX_MAPPING, &tx_mapping[0], IQS_TX_SIZE);
  if (err != IQS_OK)
  {
      LOG_ERR("Failed to set Tx mapping");
      return err;
  }

  err = iqs5xx_configure_active_channels();
  if (err != IQS_OK)
  {
      LOG_ERR("Failed to configure active channels");
      return err;
  }

  return IQS_OK;
}


iqs_error_e iqs_init(void)
{
    iqs_run_full_diagnostics();
//    iqs_run_hardware_tests();
  // Test just Tx0 + Rx0
//  uint16_t working_tx, working_rx;
//  iqs_scan_all_channels(&working_tx, &working_rx);

  // working_tx = bitmask of working Tx (bit 0 = Tx0, bit 14 = Tx14)
  // working_rx = bitmask of working Rx (bit 0 = Rx0, bit 9 = Rx9)

//    iqs_device_info_t info = {0};
//    iqs_error_e err;
//    iqs_channel_config_t channel = {0};
//
//    LOG_INFO("=== Starting IQS5xx Initialization ===");
//
//    LOG_INFO("Step 1: Acknowledging reset...");
//
//    err = iqs_reset();
//    if (err != IQS_OK)
//    {
//        LOG_ERR("Reset failed : %s", iqs_error_to_str(err));
//        return err;
//    }
//
////    err = iqs_ack_reset();
////    if (err != IQS_OK)
////    {
////        LOG_ERR("Initialization failed at reset acknowledgment: %s", iqs_error_to_str(err));
////        return err;
////    }
//
//
//    sl_udelay_wait(5000);  // 5ms
//
//    LOG_INFO("Step 2: Reading device information...");
//    err = iqs_get_device_info(&info);
//    if (err != IQS_OK){
//        LOG_ERR("Failed to get device info: %s", iqs_error_to_str(err));
//        return err;
//    }
//    LOG_INFO("Device Info - Product: 0x%04X, Project: 0x%04X", info.product_num, info.project_num);
//
//    LOG_INFO("Step 3: Channel Configuration...");
//    iqs_get_default_channel_config(&channel);
//    err = iqs_configure_channels(&channel);
//    if(err != IQS_OK){
//        LOG_ERR("Failed configuring channels : %s", iqs_error_to_str(err));
//    }
//
//    LOG_INFO("Step 4: Auto ATI...");
//    err = iqs_auto_ATI_with_retry();
//    if(err != IQS_OK){
//        LOG_ERR("Failed to set AUTO ATI bit in system control 0 reg : %s", iqs_error_to_str(err));
//        return err;
//    }
//
//    LOG_INFO("Step 5: Acknowledge Reset after Auto ATI ...");
//    err = iqs_ack_reset();
//    if (err != IQS_OK){
//        LOG_ERR("Initialization failed at reset acknowledgment: %s", iqs_error_to_str(err));
//        return err;
//    }
//
//    LOG_INFO("Step 6 : Set events (event mode | touch mode");
//    iqs_fix_stuck_touch_project56();
//    // Minimal version — works 100% on your chip
//    uint8_t mask;
//    mask = 0x0001;
//    iqs_write_register(0x0400, &mask, 2);   // Enter Event Mode
//
//    sl_udelay_wait(5000);  // 5ms
//    mask = 0xFF;  iqs_write_register(0x0501, &mask, 1);   // Proximity + Touch events (CH0–CH7)
////    err = iqs_sys_config();
////    if(err != IQS_OK){
////        LOG_ERR("Failed to set events in system config register : %s", iqs_error_to_str(err));
////        return err;
////    }
//
////    LOG_INFO("Step 6 : Enable Gestures in gesture enable register");
////    err = iqs_gesture_enable();
////    if(err != IQS_OK){
////        LOG_ERR("Failed to enable gestures in gesture enable register : %s", iqs_error_to_str(err));
////        return err;
////    }
//
//    LOG_INFO("Step 7 : Check Threshold");
//    err = iqs_check_threshold();
//    if(err != IQS_OK){
//        LOG_ERR("Failed to read thresholds : %s", iqs_error_to_str(err));
//                return err;
//    }
//
//    LOG_INFO("=== IQS5xx Initialization Complete ===");

    return IQS_OK;
}

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

    // Parse device info from buffer
    info->product_num = (data[1] << 8) | data[0];  // Bytes 0-1: Product Number
    info->project_num = (data[2] << 8) | data[3];  // Bytes 2-3: Project Number
    info -> major_ver = data[4];
    info -> minor_ver = data[5];


//    LOG_INFO("  info->Product Number: 0x%02X,0x%02X", data[0], data[1]);
//    LOG_INFO("  info->project_ Number: 0x%02X, 0x%02X", data[2], data[3]);

    LOG_INFO("Device Info Retrieved:");
    LOG_INFO("  Product Number: 0x%04X", info->product_num);
    LOG_INFO("  Project Number: 0x%04X", info->project_num);
    LOG_INFO("  Major Number: 0x%02X", info->major_ver);
    LOG_INFO("  Minor Number: 0x%02X", info->minor_ver);

    // Validate product number
    bool valid_product = (info->product_num == IQS550_PRODUCT_NUM) ||
                         (info->product_num == IQS572_PRODUCT_NUM);

    if (!valid_product)
    {
        LOG_ERR("Invalid product number! Expected 0x%04X or 0x%04X, got 0x%04X",
                IQS550_PRODUCT_NUM, IQS572_PRODUCT_NUM, info->product_num);
        return IQS_ERR_VERSION_CHECK_FAIL;
    }

    // Validate project number
    if (info->project_num != IQS_PROJECT_NUM)
    {
        LOG_WARN("Project number mismatch! Expected 0x%04X, got 0x%04X",
                 IQS_PROJECT_NUM, info->project_num);
        // Don't fail on project number mismatch, just warn
    }

    LOG_INFO("Device identification successful");
    return IQS_OK;
}

iqs_error_e iqs_end_of_communication(void)
{
    iqs_error_e err;
    uint8_t end_byte = 0x00;  // End communication byte

//    LOG_DBG("Ending communication window (write to 0x%04X)...", IQS_END_OF_COMMUNICATION);

    err = iqs_write_register(IQS_END_OF_COMMUNICATION, &end_byte, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to end communication: %s", iqs_error_to_str(err));
        return err;
    }

//    LOG_DBG("Communication window closed successfully");

    // Small delay to ensure IQS5xx processes the command
    sl_udelay_wait(500);  // 500μs delay

    return IQS_OK;
}

iqs_error_e iqs_get_rel_coordinates(iqs_coordinates_t *coords)
{
    iqs_error_e err;
    uint8_t coord_data[4] = {0};  // Read both X and Y in one transaction

    if (coords == NULL)
    {
        LOG_ERR("Invalid parameter: coords pointer is NULL");
        return IQS_ERR_INVALID_PARAM;
    }

    // Initialize coords to zero
    memset(coords, 0, sizeof(iqs_coordinates_t));

    LOG_INFO("Reading relative coordinates from 0x%04X...", IQS_REG_X_COORD);

    // Read X and Y coordinates together (more efficient)
    // Assuming registers are contiguous: X at REG_X_COORD, Y at REG_X_COORD+2
    err = iqs_read_register(IQS_REG_X_COORD, coord_data, 4);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to read coordinates: %s", iqs_error_to_str(err));
        return err;
    }
    LOG_INFO("RAW data of x : %d %d y : %d %d", coord_data[0], coord_data[1], coord_data[2], coord_data[3]);

    // Parse coordinates (assuming little-endian or based on your device)
    // IQS5xx typically uses big-endian for 16-bit values
    coords->x = (int16_t)((coord_data[0] << 8) | coord_data[1]);  // X: MSB, LSB
    coords->y = (int16_t)((coord_data[2] << 8) | coord_data[3]);  // Y: MSB, LSB

    LOG_INFO("Relative coord: X=%d, Y=%d", coords->x, coords->y);

    // Validate coordinates (optional sanity check)
    if (coords->x == 0 && coords->y == 0)
    {
        LOG_INFO("Zero coordinates detected (may indicate no movement)");
    }

    return IQS_OK;
}

iqs_error_e iqs_read_all_events(iqs_full_events_t *events)
{

  iqs_error_e err;

  uint8_t event_buffer[44] = {0};  // Read entire event block

  if (events == NULL)
  {
      LOG_ERR("Invalid parameter: events pointer is NULL");
      return IQS_ERR_INVALID_PARAM;
  }

  // Read entire event block in ONE I2C transaction (critical for event clearing!)
  err = iqs_read_register(IQS_REG_GESTURE_EVENTS_0, event_buffer, 44);
  if (err != IQS_OK)
  {
      LOG_ERR("Failed to read event block: %s", iqs_error_to_str(err));
      return err;
  }

  // Parse Gesture Events (0x000E-0x000F) - offsets 1-2 from 0x000D
  events->gesture_events_0 = event_buffer[0];
  events->gesture_events_1 = event_buffer[1];

  // Parse System Info (0x0010-0x0011) - offsets 3-4 from 0x000D
  events->system_info_0 = event_buffer[2];
  events->system_info_1 = event_buffer[3];


  // Parse touch info
  events->num_fingers = event_buffer[4];  // 0x0012

  // Parse coordinates (0x0013-0x0016) - offsets 6-9 from 0x000D
  events->rel_x = (int16_t)((event_buffer[5] << 8) | event_buffer[6]);
  events->rel_y = (int16_t)((event_buffer[7] << 8) | event_buffer[8]);

  events->abs_x = (int16_t)((event_buffer[9] << 8) | event_buffer[10]);
  events->abs_y = (int16_t)((event_buffer[11] << 8) | event_buffer[12]);

  events -> single_tap = false;
  events -> press_and_hold = false;
  events -> double_tap = false;
  events -> scroll = false;
  events -> zoom = false;

  LOG_INFO("System Info 0=0x%02X, 1=0x%02X",
           events->system_info_0,  events->system_info_1);

  LOG_INFO("Gesture Events: 0x%02X 0x%02X",
           events->gesture_events_0, events->gesture_events_1);

  LOG_INFO("Touch Details: Fingers=%d, RelX=%d, RelY=%d AbsX=%d, AbsY=%d",
           events->num_fingers, events->rel_x, events->rel_y, events -> abs_x, events -> abs_y);

  // Decode gesture events
  if (events->gesture_events_0 != 0 || events->gesture_events_1 != 0 )
  {
      LOG_INFO("Gestures detected: 0x%02X  0x%02X", events->gesture_events_0,events->gesture_events_1 );

      if (events->gesture_events_0 & IQS_SINGLE_TAP) {
          events -> single_tap = true;
          LOG_INFO("  - Single Tap");
      }
      if (events->gesture_events_0 & IQS_PRESS_AND_HOLD) {
          events -> press_and_hold = true;
          LOG_INFO("  - IQS_PRESS_AND_HOLD");
      }
      if (events->gesture_events_0 & IQS_SWIPE_Xn) {
          LOG_INFO("  - IQS_SWIPE_Xn");
      }
      if (events->gesture_events_0 & IQS_SWIPE_Xp) {
          LOG_INFO("  - IQS_SWIPE_Xp ");
      }
      if (events->gesture_events_0 & IQS_SWIPE_Yp) {
          LOG_INFO("  - IQS_SWIPE_Yp");
      }
      if (events->gesture_events_0 & IQS_SWIPE_Yn) {
          LOG_INFO("  - IQS_SWIPE_Yn");
      }

      if (events->gesture_events_1 & IQS_2_FINGER_TAP) {
          events -> double_tap = true;
          LOG_INFO("  - IQS_2_FINGER_TAP");
      }
      if (events->gesture_events_1 & IQS_SCROLL) {
          events -> scroll = true;
          LOG_INFO("  - IQS_SCROLL");
      }
      if (events->gesture_events_1 & IQS_ZOOM) {
          events -> zoom = true;
          LOG_INFO("  - IQS_ZOOM");
      }

  }
  return IQS_OK;
}






iqs_error_e iqs_read_delta_values(uint8_t *delta_buf)
{
  iqs_error_e err;
    if (delta_buf == NULL) {
        return IQS_ERR_INVALID_PARAM;
    }

    uint8_t addr_buf[2];

    // 16-bit register address, big-endian
    addr_buf[0] = (uint8_t)((IQS_DELTA_START_ADDR >> 8) & 0xFF);   // high byte
    addr_buf[1] = (uint8_t)(IQS_DELTA_START_ADDR & 0xFF);          // low byte

    // Write register address (no stop) + read 300 bytes
    err = iqs_read_register(IQS_DELTA_START_ADDR, delta_buf, IQS_DELTA_LENGTH);

    if (err != IQS_OK) {
        return IQS_ERR_I2C_READ_FAIL;
    }

    return IQS_OK;
}

iqs_error_e iqs_set_total_tx_rx(void){
  iqs_error_e err;

  uint8_t total_tx = IQS_TX_SIZE;
  err = iqs_write_register(IQS_REG_TOTAL_TX, &total_tx, 1);
  if (err != IQS_OK) {
      LOG_ERR("Total no. of Tx not set : %s", iqs_error_to_str(err));
      return IQS_ERR_I2C_READ_FAIL;
  }

  uint8_t total_rx = IQS_RX_SIZE;
  err = iqs_write_register(IQS_REG_TOTAL_RX, &total_rx, 1);
  if (err != IQS_OK) {
      LOG_ERR("Total no. of Rx not set : %s", iqs_error_to_str(err));
      return IQS_ERR_I2C_READ_FAIL;
  }
  return IQS_OK;
}



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
    sl_udelay_wait(5000);
    err = iqs_write_register(IQS_REG_TOTAL_TX, &total_tx, 1);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set Total Tx");
        return err;
    }
    sl_udelay_wait(5000);
    // Step 2: Set Rx mapping (0x063F, 10 bytes)
    err = iqs_write_register(IQS_REG_RX_MAPPING, (uint8_t *)config->rx_mapping, config->total_rx);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set Rx mapping");
        return err;
    }
    LOG_DBG("Rx mapping set");
    sl_udelay_wait(5000);
    // Step 3: Set Tx mapping (0x0649, 15 bytes)
    err = iqs_write_register(IQS_REG_TX_MAPPING, (uint8_t *)config->tx_mapping, config->total_tx);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set Tx mapping");
        return err;
    }
    LOG_DBG("Tx mapping set");
    sl_udelay_wait(5000);
    // Step 4: Set resolution
    err = iqs_set_resolution(config->x_resolution, config->y_resolution);
    if (err != IQS_OK)
    {
        LOG_ERR("Failed to set resolution");
        return err;
    }

    // Step 5: Set XY config (axis orientation)
    // no need (keep by default)

    LOG_INFO("Channel configuration complete");
    return IQS_OK;
}

iqs_error_e iqs5xx_configure_active_channels(void)
{
    uint8_t active_channels[IQS5XX_ACTIVE_CHANNELS_SIZE];

    // Each Tx row has 2 bytes (16 bits), but only bits 0-9 are used for Rx0-Rx9
    // Bit layout per row: [High byte: ------ Rx9 Rx8] [Low byte: Rx7 Rx6 Rx5 Rx4 Rx3 Rx2 Rx1 Rx0]
    // Enable RxA0-RxA9 = 0x03FF (bits 0-9 set)

    for (int tx = 0; tx < 15; tx++) {
        active_channels[tx * 2]     = 0x03;  // High byte: Rx9, Rx8 enabled
        active_channels[tx * 2 + 1] = 0xFF;  // Low byte: Rx7-Rx0 enabled
    }

    return iqs_write_register(IQS5XX_ACTIVE_CHANNELS_ADDR, active_channels, IQS5XX_ACTIVE_CHANNELS_SIZE);
}





