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
#include "nrf_delay.h"
#include "gsm_context.h"
#include "hardware.h"
#include "gsm.h"
#include "app_bootloader_uart.h"
#include "init.h"
#include "ppp.h"
#include "dns.h"
#include "app_bootloader_define.h"
#include "nrf_nvmc.h"
#include "nrf_delay.h"
#include "nrf_bootloader_wdt.h"
#include "update_firmware.h"

volatile uint32_t m_timeout_10ms_tick = 0;
static volatile uint32_t m_sys_tick = 0;
uint32_t sys_get_tick_ms(void);

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


void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
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
	nrf_gpio_cfg_output(HW_GPIO_LED_GSM_B_PIN);
	nrf_gpio_cfg_output(HW_GPIO_LED_GSM_R_PIN);
	
	nrf_gpio_cfg_output(HW_GPIO_LED_PWR_R_PIN);
	nrf_gpio_cfg_output(HW_GPIO_LED_PWR_B_PIN);
	
	nrf_gpio_cfg_output(HW_GPIO_LED_STATUS_B_PIN);
	nrf_gpio_cfg_output(HW_GPIO_LED_STATUS_R_PIN);
    
	
	nrf_gpio_pin_clear(HW_GPIO_LED_GSM_R_PIN);	    //ON
	nrf_gpio_pin_set(HW_GPIO_LED_GSM_B_PIN);		//OFF
	
	nrf_gpio_pin_set(HW_GPIO_LED_PWR_R_PIN);		//OFF
	nrf_gpio_pin_set(HW_GPIO_LED_PWR_B_PIN);	    //OFF
	
	//LED alarm OFF
	nrf_gpio_pin_clear(HW_GPIO_LED_STATUS_R_PIN);
    nrf_gpio_pin_set(HW_GPIO_LED_STATUS_B_PIN);

}

void gsm_io_initialize()
{
    nrf_gpio_cfg_input(HW_GSM_RI_PIN, NRF_GPIO_PIN_PULLUP);
    nrf_gpio_cfg_input(HW_GSM_NET_LIGHT_PIN, NRF_GPIO_PIN_PULLUP);
    nrf_gpio_cfg_input(HW_GSM_STATUS_PIN, NRF_GPIO_PIN_PULLDOWN);

    nrf_gpio_cfg_output(GSM_MUTE_PIN_NUMBER);
    nrf_gpio_pin_write(GSM_MUTE_PIN_NUMBER, 0);

    //VBAT GSM
    nrf_gpio_cfg_output(HW_GSM_PWR_CTRL_PIN);
    nrf_gpio_pin_write(HW_GSM_PWR_CTRL_PIN, 0);
    nrf_delay_ms(100);
    nrf_gpio_pin_write(HW_GSM_PWR_CTRL_PIN, 1);

    //Power ON module
    nrf_gpio_cfg_output(HW_GSM_POWER_KEY_PIN);
    nrf_gpio_pin_write(HW_GSM_POWER_KEY_PIN, 0);		
}

void gsm_gpio_set(uint32_t pin, bool on_off)
{
    if (on_off)
        nrf_gpio_pin_set(pin);
    else
        nrf_gpio_pin_clear(pin);
}

bool gsm_gpio_get(uint32_t pin)
{
    return nrf_gpio_pin_read(pin) ? true : false;
}

static void board_clock_initialize( void )
{
	// Start HFCLK and wait for it to start.
	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);
}

static void task_10ms(void) 
{	
    gsm_hw_polling();
    static uint32_t j = 0;
    if (j++ == 10)
    {
        update_fw_polling_download_status();
        j = 0;
    }
    
    gsm_state_polling_task();
}

void do_ota_update(void)
{
    board_clock_initialize();
    board_gpio_initialize();
        
    lwip_init();

    ip_addr_t dns_server_0 = IPADDR4_INIT_BYTES(8, 8, 8, 8);
    ip_addr_t dns_server_1 = IPADDR4_INIT_BYTES(1, 1, 1, 1);
    dns_setserver(0, &dns_server_0);
    dns_setserver(1, &dns_server_1);
    dns_init();
    
    update_fw_initialize();

    gsm_data_layer_initialize();
    
    gsm_hw_config_t gsm_conf;
    gsm_conf.gpio_initialize = gsm_io_initialize;
    gsm_conf.io_set = gsm_gpio_set;
    gsm_conf.io_get = gsm_gpio_get;
    gsm_conf.delay = nrf_delay_ms;
    gsm_conf.uart_initialize = app_bootloader_uart_initialize;
    gsm_conf.gpio.power_key = HW_GSM_POWER_KEY_PIN;
    gsm_conf.gpio.status_pin = HW_GSM_STATUS_PIN;
    gsm_conf.serial_tx = app_bootloader_uart_write;
    gsm_conf.serial_rx_flush = app_bootloader_rx_flush;
    gsm_conf.sys_now = sys_get_tick_ms;
    gsm_conf.hw_polling_ms = 100;   // should be 100ms
    gsm_hw_initialize(&gsm_conf);
    
    nrf_drv_systick_init();

    SysTick_Config(SystemCoreClock / 1000);
    NVIC_EnableIRQ(SysTick_IRQn);
    uint32_t ota_timeout = 0;

    while(1)
    {
        if (gsm_data_layer_is_ppp_connected())
        {
            nrf_gpio_pin_clear(HW_GPIO_LED_GSM_B_PIN);	    //ON
            nrf_gpio_pin_set(HW_GPIO_LED_GSM_R_PIN);	    //OFF

            static bool initialize = false;
            if (initialize == false)
            {
                initialize = true;
                /* Get OTA information stored in flash memory
                * Main application must be write ota information to this address
                * After reset, bootloader will check ota information and do ota process
                */
                app_ota_info_t * ota_config = (app_ota_info_t*)APPLICATION_OTA_INFORMATION;
                
                /* Enable HTTP and Download FW */
                gsm_ctx()->file_transfer.state = FT_WAIT_TRANSFER_STATE;

                /* Get http download URL information, HTTPS not supported */
                /* Link http://abcefe.com/ota_image.bin =>> HTTP_Server =  abcefe.com, HTTP_File = /ota_image.bin*/

                snprintf((char*)gsm_ctx()->file_transfer.server, sizeof(gsm_ctx()->file_transfer.server), 
                        "%s", 
                        ota_config->http_server);
                
                gsm_ctx()->file_transfer.port = ota_config->http_port;

                snprintf((char*)gsm_ctx()->file_transfer.file, sizeof(gsm_ctx()->file_transfer.file), 
                        "%s", 
                        ota_config->http_file);
            }
        }
        else
        {
            nrf_gpio_pin_clear(HW_GPIO_LED_GSM_R_PIN);	    
            nrf_gpio_pin_set(HW_GPIO_LED_GSM_B_PIN);	    
        }
        
        if(m_timeout_10ms_tick >= 10) 
		{
			task_10ms();
            m_timeout_10ms_tick = 0;

            /* Blink indicator led when device in bootloader state */
            static uint32_t j = 0;
            if (j++ == 5)
            {
                nrf_gpio_pin_toggle(HW_GPIO_LED_STATUS_R_PIN);
                nrf_gpio_pin_toggle(HW_GPIO_LED_STATUS_B_PIN);  
                j = 0;
            }
		}
        
        if (sys_get_tick_ms() - ota_timeout > 1000000)
        {
            ota_timeout = sys_get_tick_ms();
            NVIC_SystemReset();
        }

        gsm_hw_lwip_polling_task();
        update_fw_http_polling();
    }
}

int main(void)
{        
    (void) NRF_LOG_INIT(nrf_bootloader_dfu_timer_counter_get);
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    NRF_LOG_INFO("Bootloader %s", __DATE__);
    NRF_LOG_FLUSH();
    
    app_ota_info_t * ota_config = (app_ota_info_t*)APPLICATION_OTA_INFORMATION;
    if (ota_config->ota_flag == APP_BOOTLOADER_OTA)
    {
        nrf_bootloader_wdt_init();
        do_ota_update();
    }
    else
    {
        nrf_gpio_cfg_output(HW_GPIO_LED_STATUS_B_PIN);
        nrf_gpio_cfg_output(HW_GPIO_LED_STATUS_R_PIN);
        
        NRF_LOG_INFO("Run application");
        
        for (uint32_t i = 0; i < 4 ; i++)
        {
            nrf_gpio_pin_toggle(HW_GPIO_LED_STATUS_B_PIN);
            nrf_gpio_pin_toggle(HW_GPIO_LED_STATUS_R_PIN);
            nrf_delay_ms(10);
        }

        NRF_LOG_FLUSH();
        nrf_bootloader_app_start();
    }
    while(1)
    {
        // Should never get there
        on_error();
    }
}

/**
 * @brief Override LWIP weak function
 */
uint32_t sys_now(void)
{
    return m_sys_tick;
}

/**
 * @brief Override LWIP weak function
 */
uint32_t sys_jiffies(void)
{
	return m_sys_tick;
}

/** 
 * @brief SysTick interrupt handler
 */
void SysTick_Handler(void)  
{
    m_sys_tick++;
    if (m_sys_tick % 10 == 0)
    {
        m_timeout_10ms_tick++;
    }
}


uint32_t sys_get_tick_ms(void)
{
    return m_sys_tick;
}

void app_bootloader_set_ota_done(char * debug_info)
{
    nrf_nvmc_page_erase(APPLICATION_OTA_INFORMATION);
    app_ota_info_t ota_config;
    memset(&ota_config, 0, sizeof(app_ota_info_t));
    ota_config.ota_flag = APP_BOOTLOADER_NORMAL;
    snprintf((char*)ota_config.debug_info, sizeof(ota_config.debug_info), "%s", debug_info);
    nrf_nvmc_write_bytes(APPLICATION_OTA_INFORMATION, (uint8_t*)&ota_config, sizeof(app_ota_info_t));
}

/*
void nrf_bootloader_app_start_final_clone(uint32_t vector_table_addr)
{
    // HuyTV
    ret_code_t ret_val;

    // Size of the flash area to protect.
    uint32_t area_size;    
    
    // protect bootloader
    area_size = BOOTLOADER_SIZE;

    ret_val = nrf_bootloader_flash_protect(BOOTLOADER_START_ADDR, area_size);

    if (ret_val != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Could not protect bootloader and settings pages, 0x%x. Bootloader addr 0x%08X, size 0x%08X", 
                ret_val,
                BOOTLOADER_START_ADDR,
                area_size);
        NRF_LOG_FLUSH();
    }
    APP_ERROR_CHECK(ret_val);
    
    // Protect MBR parameter storage + Bootloader settings
    area_size = BOOTLOADER_SETTINGS_PAGE_SIZE + NRF_MBR_PARAMS_PAGE_SIZE;

    ret_val = nrf_bootloader_flash_protect(NRF_MBR_PARAMS_PAGE_ADDRESS, area_size);

    if (ret_val != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Could not protect mbr and bootloader settings pages, 0x%x. Bootloader addr 0x%08X, size 0x%08X", 
                ret_val,
                NRF_MBR_PARAMS_PAGE_ADDRESS,
                area_size);
        NRF_LOG_FLUSH();
    }
    APP_ERROR_CHECK(ret_val);
    
    
    
    // 
    ret_val = nrf_bootloader_flash_protect(0,
                    nrf_dfu_bank0_start_addr() + ALIGN_TO_PAGE(s_dfu_settings.bank_0.image_size));

    if (ret_val != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Could not protect SoftDevice and application, 0x%x. Addr 0x%08X", 
                ret_val,
                nrf_dfu_bank0_start_addr() + ALIGN_TO_PAGE(s_dfu_settings.bank_0.image_size));
        NRF_LOG_FLUSH();
    }
    APP_ERROR_CHECK(ret_val);

    // Run application
    app_start(vector_table_addr);
}
*/

