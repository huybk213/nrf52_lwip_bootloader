#ifndef __GSM_H__
#define __GSM_H__

#include "gsm_context.h"
#include <stdbool.h>
#include <stdint.h>
#include "ppp.h"

/* AT Command lists */
#define AT_ECHO_OFF "ATE0\r"
#define AT_BAUDRATE_115200 "AT+IPR=115200\r"
#define AT_REPORT_EQUIPMENT_ERROR_ENABLE "AT+CMEE=2\r"
#define AT_NEW_MESSSAGE_INDICATION "AT+CNMI=2,1,0,0,0\r"
#define AT_CPIN "AT+CPIN?\r"
#define AT_GSN "AT+GSN\r"
#define AT_CFUN "AT+CFUN=1\r"
#define AT_CMGF "AT+CMGF=1\r"
#define AT_CIMI "AT+CIMI\r"
#define AT_CSQ "AT+CSQ\r"
#define AT_CGREG "AT+CGREG=1\r"
#define AT_SETUP_APN "AT+CGDCONT=1,\"IP\",\"v-internet\"\r"
#define ATV1 "ATV1\r"
#define ATO "ATO\r"
#define AT_CONNTECT "CONNECT"
#define AT_ENTER_PPP "ATD*99***1#\r"
#define AT_COPS "AT+COPS?\r"
#define AT_SWITCH_PPP_TO_AT_MODE "+++"
#define AT_OK "OK"
#define GSM_NO_CARRIER "NO CARRIER"
#define GSM_SIM_NOT_INSERT "+CPIN: NOT INSERTED"
typedef enum
{
    GSM_IMEI_MODULE = 0x00,
    GSM_IMEI_SIM
} gsm_imei_t;
typedef enum
{
    GSM_EVEN_OK = 0,
    GSM_EVEN_TIMEOUT,
    GSM_EVEN_ERROR
} gsm_response_evt_t;

typedef enum
{
    GSM_OK = 0,
    GSM_STATE_RESET,
    GSM_STATE_PWR_ON,
    GSM_STATE_CLOSE_PPP,
    GSM_REOPEN_PPP
} gsm_state_t;

typedef enum
{
    GSM_AT_MODE = 0,
    GSM_PPP_MODE
} gsm_at_mode_t;

typedef struct
{
    gsm_state_t state;
    gsm_at_mode_t mode;
    uint8_t step;
    uint8_t ready;
    uint8_t ppp_phase; // @ref lwip ppp.h
} gsm_manager_t;

typedef struct
{
    uint32_t power_key;
    uint32_t status_pin;
} gsm_gpio_t;

/**
 * @brief GSM power on success callback
 */
typedef void (*gsm_power_on_cb_t)(void);

/**
 * @brief GSM IO control callback
 * @param[in] pin Pin number
 * @param[in] on_off Pin level
 */
typedef void (*gsm_hw_io_set_cb)(uint32_t pin, bool on_off);

/**
 * @brief GSM IO read callback
 * @param[in] pin Pin number
 * @retval Pin level
 */
typedef bool (*gsm_hw_io_get_cb)(uint32_t pin);

/**
 * @brief GSM delay callback
 */
typedef void (*gsm_hw_delay_cb)(uint32_t ms);

/**
 * @brief GSM serial tx data callback
 * @param[in] data Pointer to sending buffer
 * @param[in] len Number of bytes send to serial port
 * @retval Number of bytes written
 */
typedef uint32_t (*gsm_hw_uart_tx)(uint8_t *data, uint32_t len);

/**
 * @brief GSM serial rx data callback
 * @param[out] data Pointer to read buffer
 * @param[in] len Number of bytes want to from serial port
 * @retval Number of bytes read
 */
typedef uint16_t (*gsm_hw_uart_rx)(uint8_t *data, uint16_t len);

/**
 * @brief GSM flush all data in serial port
 */
typedef void (*gsm_hw_uart_rx_flush)(void);

/**
 * @brief GSM serial initialize callback
 */
typedef void (*gsm_hw_uart_initialize_cb)(void);

/**
 * @brief GSM GPIO initialize callback
 */
typedef void (*gsm_hw_gpio_initialize)(void);

/**
 * @brief GSM get current system tick in milisecond
 * @retval System tick in miliseconds
 */
typedef uint32_t (*gsm_sys_now)(void);

typedef struct
{
    gsm_hw_delay_cb delay;
    gsm_hw_io_set_cb io_set;
    gsm_hw_io_get_cb io_get;
    gsm_hw_uart_initialize_cb uart_initialize;
    gsm_hw_gpio_initialize gpio_initialize;
    gsm_hw_uart_tx serial_tx;
    gsm_hw_uart_rx serial_rx;
    gsm_hw_uart_rx_flush serial_rx_flush;
    gsm_gpio_t gpio;
    gsm_sys_now sys_now;
    uint32_t hw_polling_ms; // Should be 100ms
} gsm_hw_config_t;

typedef void (*gsm_send_at_cb_t)(gsm_response_evt_t event, void *response_buffer);

/**
 * @brief GSM hardware polling task
 * @note Shoud polling every 100ms
 */
void gsm_hw_polling_task(void);

/**
 * @brief Get PPP connection status
 * @retval TRUE PPP stack is connected
 *         FALSE PPP stack is not connected
 */
bool gsm_data_layer_is_ppp_connected(void);

/**
 * @brief Send AT command to GSM moderm
 * @param[in] cmd Command send to moderm
 * @param[in] expected_response Response expectation
 * @param[in] timeout_ms timeout in milisecond wait for response from moderm
 * @param[in] retry_count Number of resend at command
 * @param]in] Callback to process response data or timeout event
 */
void gsm_hw_send_at_cmd(char *cmd, char *expect_response, uint16_t timeout_ms,
                        uint8_t retry_count, gsm_send_at_cb_t callback);

/**
 * @brief Initialize gsm hardware
 * @param[in] gsm_conf Pointer to gsm configuration
 */
void gsm_hw_initialize(gsm_hw_config_t *gsm_conf);

/**
 * @brief Initialize gsm data layer
 */
void gsm_data_layer_initialize(void);

/**
 * @brief Polling gsm data layer state
 */
void gsm_state_polling_task(void);

/**
 * @brief Main lwip task
 */
void gsm_hw_lwip_polling_task(void);

/**
 * @brief Get gsm hardware configuration function
 * @return Pointer to gsm configuration
 */
gsm_hw_config_t *gsm_hw_get_config(void);

/**
 * @brief Insert data into gsm buffer
 * @param data Data received from uart port
 */
void gsm_uart_handler(uint8_t data);

/**
 * @brief Get gsm manager config
 * @return Pointer to gsm manger
 */
gsm_manager_t *gsm_manager_ctx(void);

/**
 * @brief Hardware reset module GSM by power key
 * @param[i] cb GSM power on success callback
 */
void gsm_hw_hardreset(gsm_power_on_cb_t cb);

/**
 * @brief Set current gsm at mode
 * @param[in] mode AT mode or PPP mode
 */
void gsm_set_at_mode(gsm_at_mode_t mode);

/**
 * @brief Get PPP control block
 * @retval Pointer to PPP control block
 */
ppp_pcb *gsm_data_layer_get_ppp_control_block(void);

#endif // __GSM_H__
