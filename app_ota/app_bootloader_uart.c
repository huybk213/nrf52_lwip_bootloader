#include "app_bootloader_uart.h"
#include "app_bootloader_uart.h"
#include "nrf_uart.h"
#include "app_bootloader_define.h"
#include "nrf_error.h"
#include "hardware.h"
#include "nrf.h"
#include "app_uart.h"
#include "nrf_log.h"
#include "nrf_gpio.h"

#if defined NRF52840_XXAA || defined NRF52833_XXAA
#include "nrf52840_peripherals.h"
#else
#include "nrf52832_peripherals.h"
#endif

#include "gsm.h"

void UART_IRQHandler(app_uart_evt_t *p_event)
{
    uint8_t uart_receiver = 0;

    switch (p_event->evt_type)
    {
    case APP_UART_DATA_READY:
        // time_uart_free = 0;
        if (app_uart_get((uint8_t *)&uart_receiver) == NRF_SUCCESS)
        {
            gsm_uart_handler(uart_receiver);
        }
        break;

    default:
        break;
    }
}

void app_bootloader_uart_initialize(void)
{
    static bool initialized = false;
    if (initialized)
        return;

    initialized = true;
    uint32_t err_code;
    const app_uart_comm_params_t comm_params =
    {
        HW_GSM_RX_PIN_NUMBER,
        HW_GSM_TX_PIN_NUMBER,
        HW_GSM_RTS_PIN_NUMBER,
        HW_GSM_CTS_PIN_NUMBER,
        HW_GSM_UART_HWFC,
        false,
#if defined(UART_PRESENT)
        //         NRF_UART_BAUDRATE_9600
        NRF_UART_BAUDRATE_115200
#else
        NRF_UARTE_BAUDRATE_115200
#endif
    };

    APP_UART_FIFO_INIT(&comm_params,
                       UART_RX_BUF_SIZE,
                       UART_TX_BUF_SIZE,
                       UART_IRQHandler,
                       APP_IRQ_PRIORITY_LOWEST,
                       err_code);

    APP_ERROR_CHECK(err_code);
}

uint32_t app_bootloader_uart_write(uint8_t *data, uint32_t length)
{
    if (data == NULL || length == 0)
        return 0;

    uint32_t err;
    for (uint32_t i = 0; i < length; i++)
    {
        while((err = app_uart_put(*(data + i))) != NRF_SUCCESS);
        //err = app_uart_put(*(data + i));
        //if (err != NRF_SUCCESS)
        //{
        //    NRF_LOG_INFO("UART TX error %d, idx %d\r\n", err, i);
        //    APP_ERROR_CHECK(err);
        //    return i;
        //}
    }

    return length;
}

uint32_t app_bootloader_uart_read(uint8_t *data, uint32_t length)
{
    if (data == NULL)
        return 0;

    uint32_t index = 0;
    int32_t ret = NRF_SUCCESS;
    while (length--)
    {
        ret = app_uart_get(&data[index++]);
    }
    return ret;
}

void app_bootloader_rx_flush(void)
{
    uint8_t tmp;
    while (app_uart_get(&tmp) == NRF_SUCCESS);
}
