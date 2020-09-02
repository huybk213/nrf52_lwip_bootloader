#ifndef UPDATE_FIRMWARE_H
#define UPDATE_FIRMWARE_H

#include "gsm_context.h"


/**
 * @brief HTTP download task
 */
void update_fw_http_polling(void);

/**
 * @brief Polling file download status
 */
void update_fw_polling_download_status(void);

/**
 * @brief Update firmware initialize
 */
void update_fw_initialize(void);

#endif /* UPDATE_FIRMWARE_H */

