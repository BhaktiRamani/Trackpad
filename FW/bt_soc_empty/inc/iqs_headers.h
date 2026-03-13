/*
 * iqs_headers.h
 *
 *  Created on: Nov 28, 2025
 *      Author: Bhakti Ramani
 */

#ifndef INC_IQS_HEADERS_H_
#define INC_IQS_HEADERS_H_

typedef enum {
  IQS_OK = 0,                 ///< Operation successful
  IQS_ERR_I2C_READ_FAIL,          ///< I2C read communication failed
  IQS_ERR_I2C_WRITE_FAIL,          ///< I2C write communication failed
  IQS_ERR_INVALID_PARAM,     ///< Invalid parameter
  IQS_ERR_VERSION_CHECK_FAIL,  // Initial i2c communication failed
  IQS_ERR_NOT_INITIALIZED,   ///< Driver not initialized
  IQS_ERR_TIMEOUT,           ///< Operation timeout
  IQS_ERR_NO_TOUCH,           ///< No touch detected
  IQS_GESTURE_EN_FAIL,
  IQS_ERR_ATI_FAILED,
  IQS_ERROR_ATI_FAILED,
  IQS_ERROR_TIMEOUT
} iqs_error_e;

#endif /* INC_IQS_HEADERS_H_ */
