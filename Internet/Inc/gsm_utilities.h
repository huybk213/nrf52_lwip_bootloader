#ifndef GSM_UTILITIES_H
#define GSM_UTILITIES_H

#include "gsm_context.h"

/**
 * @brief Get gsm imei from buffer
 * @param[in] IMEI raw buffer from gsm module
 * @param[out] IMEI result
 */ 
void gsm_utilities_get_imei(uint8_t *imei_buffer, uint8_t * result);

/**
 * @brief Get signal strength from buffer
 * @param[in] buffer buffer response from GSM module
 * @param[out] CSQ value
 * @note buffer = "+CSQ:..."
 * @retval TRUE Get csq success
 *         FALSE Get csq fail
 */ 
bool gsm_utilities_get_signal_strength_from_buffer(char * buffer, uint8_t * csq);

/**
 * @brief Get Number from string
 * @param[in] Index Begin index of buffer want to find
 * @param[i] buffer Data want to search
 * @note buffer = "abc124mff" =>> gsm_utilities_get_number_from_string(3, buffer) = 123
 * @retval Number result from string
 */ 
uint32_t gsm_utilities_get_number_from_string(uint16_t index, char* buffer);
  
#endif // GSM_UTILITIES_H

