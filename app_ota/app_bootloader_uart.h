#ifndef APP_BOOTLOADER_UART_H
#define APP_BOOTLOADER_UART_H

#include "stdint.h"

#ifndef UART_TX_BUF_SIZE
#define UART_TX_BUF_SIZE 512
#endif //UART_TX_BUF_SIZE

#ifndef UART_RX_BUF_SIZE
#define UART_RX_BUF_SIZE 256 /**< UART RX buffer size. */
#endif                       //UART_RX_BUF_SIZE

/**
 * @brief Initialize UART driver
 */
void app_bootloader_uart_initialize(void);

/**
 * @brief Send data to uart port
 * @param[in] data Pointer to sending buffer
 * @param[in] length Buzzer size
 * @retval    Number of data written
 */
uint32_t app_bootloader_uart_write(uint8_t *data, uint32_t length);

/**
 * @brief Read data from uart port
 * @param[out] data Pointer to read buffer
 * @param[in] length Size of read data
 * @retval    Number of data read
 */
uint32_t app_bootloader_uart_read(uint8_t *data, uint32_t length);

/**
 * @brief Flush all data in fifo rx
 */
void app_bootloader_rx_flush(void);

#endif /* APP_BOOTLOADER_UART_H */
