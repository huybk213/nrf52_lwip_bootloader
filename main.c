#include <stdint.h>
#include "boards.h"
#include "nrf_mbr.h"
//#include "nrf_bootloader.h"
#include "nrf_bootloader_app_start.h"
#include "nrf_bootloader_dfu_timers.h"
#include "nrf_dfu.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "app_error.h"
#include "app_error_weak.h"
#include "nrf_drv_systick.h"
#include "nrf_drv_clock.h"
#include "nrf_delay.h"
#include "gsm_context.h"
#include "hardware.h"
#include "gsm.h"
#include "app_bootloader_uart.h"
#include "init.h"
#include "ppp.h"
#include "dns.h"
#include "nrf_nvmc.h"
#include "nrf_delay.h"
#include "main.h"
#include "mqtt_user.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "cpu_utils.h"

static TaskHandle_t m_logger_thread, m_gsm_task;  

//static volatile uint32_t m_sys_tick = 0;

static void on_error(void)
{
    NRF_LOG_FINAL_FLUSH();

#if NRF_MODULE_ENABLED(NRF_LOG_BACKEND_RTT)
    // To allow the buffer to be flushed by the host.
    nrf_delay_ms(100);
#endif
#ifdef NRF_DFU_DEBUG_VERSION
    NRF_BREAKPOINT_COND;
#endif
    NVIC_SystemReset();
}

void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t *p_file_name)
{
    NRF_LOG_ERROR("%s:%d", p_file_name, line_num);
    on_error();
}

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
    NRF_LOG_ERROR("Received a fault! id: 0x%08x, pc: 0x%08x, info: 0x%08x", id, pc, info);
    on_error();
}

void app_error_handler_bare(uint32_t error_code)
{
    NRF_LOG_ERROR("Received an error: 0x%08x!", error_code);
    on_error();
}

static void board_gpio_initialize(void)
{
#ifdef SIM800C_BOARD
    nrf_gpio_cfg_output(HW_GPIO_LED_GSM_B_PIN);
    nrf_gpio_cfg_output(HW_GPIO_LED_GSM_R_PIN);

    nrf_gpio_cfg_output(HW_GPIO_LED_PWR_R_PIN);
    nrf_gpio_cfg_output(HW_GPIO_LED_PWR_B_PIN);

    nrf_gpio_cfg_output(HW_GPIO_LED_STATUS_B_PIN);
    nrf_gpio_cfg_output(HW_GPIO_LED_STATUS_R_PIN);

    nrf_gpio_pin_clear(HW_GPIO_LED_GSM_R_PIN); //ON
    nrf_gpio_pin_set(HW_GPIO_LED_GSM_B_PIN);   //OFF

    nrf_gpio_pin_set(HW_GPIO_LED_PWR_R_PIN); //OFF
    nrf_gpio_pin_set(HW_GPIO_LED_PWR_B_PIN); //OFF

    //LED alarm OFF
    nrf_gpio_pin_clear(HW_GPIO_LED_STATUS_R_PIN);
    nrf_gpio_pin_set(HW_GPIO_LED_STATUS_B_PIN);
#endif /* SIM800C_BOARD */
}

void gsm_io_initialize()
{
    nrf_gpio_cfg_input(HW_GSM_STATUS_PIN, NRF_GPIO_PIN_PULLDOWN);

    //Power ON module
    nrf_gpio_cfg_output(HW_GSM_POWER_KEY_PIN);
    nrf_gpio_pin_write(HW_GSM_POWER_KEY_PIN, 0);
}

void gsm_gpio_set(uint32_t pin, bool on_off)
{
    if (pin != 0xFFFFFFFF)
    {
        if (on_off)
        {
            nrf_gpio_pin_set(pin);
        }
        else
        {
            nrf_gpio_pin_clear(pin);
        }
    }
}

bool gsm_gpio_get(uint32_t pin)
{
    return nrf_gpio_pin_read(pin) ? true : false;
}

static void clock_init(void)
{
    ret_code_t err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);
}

static void board_clock_initialize(void)
{
    // Start HFCLK and wait for it to start.
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART = 1;
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

    clock_init();

}

static void logger_thread(void * arg)
{
    UNUSED_PARAMETER(arg);

    while (1)
    {
        NRF_LOG_FLUSH();

        vTaskSuspend(NULL); // Suspend myself
        //vTaskDelay(1024);
        //printf(".");
    }
}

static void gsm_task(void * arg)
{
    static uint32_t tick = 0;

    while (1)
    {
        gsm_hw_polling_task();

        gsm_state_polling_task();

        gsm_hw_lwip_polling_task();

        if (sys_get_tick_ms() - tick > 1000)
        {
            // should call every 1sec
            mqtt_user_polling_task();
            tick = sys_get_tick_ms();
        }

        static uint16_t last_cpu;
        uint16_t current_cpu = osGetCPUUsage();
        if (last_cpu != current_cpu)
        {
            DebugPrint("CPU %d\r\n", current_cpu);
            last_cpu = current_cpu;
        }

        vTaskDelay(1);
    }
}

void vApplicationMallocFailedHook(void)
{
    APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
}


///**@brief A function which is hooked to idle task.
// * @note Idle hook must be enabled in FreeRTOS configuration (configUSE_IDLE_HOOK).
// */
//void vApplicationIdleHook( void )
//{
//    vTaskResume(m_logger_thread);
//}


void gsm_start(void)
{
    board_clock_initialize();
    board_gpio_initialize();

    lwip_init();

    ip_addr_t dns_server_0 = IPADDR4_INIT_BYTES(8, 8, 8, 8);
    ip_addr_t dns_server_1 = IPADDR4_INIT_BYTES(1, 1, 1, 1);
    dns_setserver(0, &dns_server_0);
    dns_setserver(1, &dns_server_1);
    dns_init();


    gsm_data_layer_initialize();

    gsm_hw_config_t gsm_conf;
    gsm_conf.gpio_initialize = gsm_io_initialize;
    gsm_conf.io_set = gsm_gpio_set;
    gsm_conf.io_get = gsm_gpio_get;
    gsm_conf.delay = nrf_delay_ms;
    gsm_conf.uart_initialize = app_bootloader_uart_initialize;
    gsm_conf.gpio.power_key = HW_GSM_POWER_KEY_PIN;
    gsm_conf.gpio.status_pin = HW_GSM_STATUS_PIN;
    gsm_conf.gpio.reset_pin = HW_GSM_RESET_PIN;
    gsm_conf.serial_tx = app_bootloader_uart_write;
    gsm_conf.serial_rx_flush = app_bootloader_rx_flush;
    gsm_conf.sys_now = sys_get_tick_ms;
    gsm_conf.hw_polling_ms = 100; // should be 100ms
    gsm_hw_initialize(&gsm_conf);

    //nrf_drv_systick_init();

    //SysTick_Config(SystemCoreClock / 1000);
    //NVIC_EnableIRQ(SysTick_IRQn);

    mqtt_set_sub_topic_name("huytv/test/123456");

    // Start execution.
    if (pdPASS != xTaskCreate(gsm_task, "gsm", 4096, NULL, 1, &m_gsm_task))
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    // Start execution.
    if (pdPASS != xTaskCreate(logger_thread, "LOGGER", 256, NULL, 1, &m_logger_thread))
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    vTaskStartScheduler();
    ASSERT(-1);
}

int main(void)
{
    (void)NRF_LOG_INIT(nrf_bootloader_dfu_timer_counter_get);
    NRF_LOG_DEFAULT_BACKENDS_INIT();
    printf("Hello world\r\n");

    //NRF_LOG_INFO("Build %s", __DATE__);
    NRF_LOG_FLUSH();
   
    gsm_start();
}

/**
 * @brief Override LWIP weak function
 */
uint32_t sys_now(void)
{
    return (xTaskGetTickCount() * 1000) / 1024;
}

/**
 * @brief Override LWIP weak function
 */
uint32_t sys_jiffies(void)
{
    return (xTaskGetTickCount() * 1000) / 1024;
}

///** 
// * @brief SysTick interrupt handler
// */
//void SysTick_Handler(void)
//{
//    m_sys_tick++;
//}

uint32_t sys_get_tick_ms(void)
{
    return  (xTaskGetTickCount() * 1000) / 1024;
}