/*
 * iqs550_new.h
 *
 *  Created on: Dec 2, 2025
 *      Author: Bhakti Ramani
 */

#ifndef INC_IQS550_NEW_H_
#define INC_IQS550_NEW_H_

#include "app.h"
#include "iqs_headers.h"

iqs_error_e iqs5xx_init(void);
iqs_error_e iqs5xx_init_v2(void);

iqs_error_e iqs5xx_clear_alp_channels(void);
iqs_error_e iqs5xx_disable_alp(void);

iqs_error_e iqs5xx_manual_ati_config(uint16_t target, uint8_t ati_c_global, uint8_t compensation_value);
#endif /* INC_IQS550_NEW_H_ */
