#include "gsm.h"
#include "hardware.h"
#include "gsm_context.h"
#include "string.h"
#include "lwip/sio.h"
#include "ppp.h"
#include "pppos.h"

#define GSM_PPP_MODEM_BUFFER_SIZE 1024
#define GSM_CLEAR_BUFFER()                       \
    {                                            \
        m_gsm_hw_buffer.recv_buffer.idx_in = 0;  \
        m_gsm_hw_buffer.recv_buffer.idx_out = 0; \
    }

typedef struct
{
    char *cmd;
    char *expect_response_from_atc;
    uint16_t timeout_atc;
    uint16_t current_timeout_atc;
    uint8_t retry_cnt_atc;
    SmallBuffer_t recv_buffer;
    gsm_send_AT_cb_t send_at_cb;
} gsm_at_cmd_t;

typedef struct
{
    uint16_t idx_in;
    uint16_t idx_out;
    uint8_t buffer[GSM_PPP_MODEM_BUFFER_SIZE];
} gsm_modem_buffer_t;

typedef struct
{
    gsm_at_cmd_t at_cmd;
    gsm_modem_buffer_t recv_buffer;
} gsm_hw_buffer_t;

static gsm_hw_buffer_t m_gsm_hw_buffer;
static uint8_t m_ppp_rx_buffer[GSM_PPP_MODEM_BUFFER_SIZE];
static volatile bool m_is_new_serial_data = 0;
static gsm_hw_config_t m_gsm_hw_config;

void gsm_hw_initialize(gsm_hw_config_t *gsm_conf)
{
    //        ASSERT(gsmConf);
    DebugPrint("GSM hardware initialize\r\n");
    memcpy(&m_gsm_hw_config, gsm_conf, sizeof(gsm_hw_config_t));

    gsm_conf->gpio_initialize();
    gsm_conf->uart_initialize();
    gsm_conf->serial_rx_flush();
    m_gsm_hw_buffer.recv_buffer.idx_in = 0;
    m_gsm_hw_buffer.recv_buffer.idx_out = 0;
}

void gsm_uart_handler(uint8_t data)
{
    if (m_gsm_hw_buffer.at_cmd.timeout_atc)
    {
        m_gsm_hw_buffer.at_cmd.recv_buffer.buffer[m_gsm_hw_buffer.at_cmd.recv_buffer.buffer_idx++] = data;
        if (m_gsm_hw_buffer.at_cmd.recv_buffer.buffer_idx >= SMALL_BUFFER_SIZE)
            m_gsm_hw_buffer.at_cmd.recv_buffer.buffer_idx = 0;

        m_gsm_hw_buffer.at_cmd.recv_buffer.buffer[m_gsm_hw_buffer.at_cmd.recv_buffer.buffer_idx] = 0;
    }
    else
    {
        m_gsm_hw_buffer.recv_buffer.buffer[m_gsm_hw_buffer.recv_buffer.idx_in++] = data;
        if (m_gsm_hw_buffer.recv_buffer.idx_in >= GSM_PPP_MODEM_BUFFER_SIZE)
            m_gsm_hw_buffer.recv_buffer.idx_in = 0;

        m_gsm_hw_buffer.recv_buffer.buffer[m_gsm_hw_buffer.recv_buffer.idx_in] = 0;
    }

    m_is_new_serial_data = true;
}

void gsm_hw_polling_task(void)
{
    static uint32_t hw_tick = 0;
    if (m_gsm_hw_config.sys_now() - hw_tick < m_gsm_hw_config.hw_polling_ms)
    {
        return;
    }

    hw_tick = m_gsm_hw_config.sys_now();

    if (m_gsm_hw_buffer.at_cmd.timeout_atc)
    {
        m_gsm_hw_buffer.at_cmd.current_timeout_atc++;

        if (m_gsm_hw_buffer.at_cmd.current_timeout_atc > m_gsm_hw_buffer.at_cmd.timeout_atc)
        {
            m_gsm_hw_buffer.at_cmd.current_timeout_atc = 0;

            if (m_gsm_hw_buffer.at_cmd.retry_cnt_atc)
            {
                m_gsm_hw_buffer.at_cmd.retry_cnt_atc--;
            }

            if (m_gsm_hw_buffer.at_cmd.retry_cnt_atc == 0)
            {
                m_gsm_hw_buffer.at_cmd.timeout_atc = 0;

                if (m_gsm_hw_buffer.at_cmd.send_at_cb != NULL)
                {
                    m_gsm_hw_buffer.at_cmd.send_at_cb(GSM_EVEN_TIMEOUT, NULL);
                }
            }
            else
            {
                DebugPrint("Resend AT : %s\r\n", m_gsm_hw_buffer.at_cmd.cmd);
                m_gsm_hw_config.serial_tx((uint8_t *)m_gsm_hw_buffer.at_cmd.cmd, strlen(m_gsm_hw_buffer.at_cmd.cmd));
            }
        }

        if (strstr((char *)(m_gsm_hw_buffer.at_cmd.recv_buffer.buffer), m_gsm_hw_buffer.at_cmd.expect_response_from_atc))
        {
            m_gsm_hw_buffer.at_cmd.timeout_atc = 0;

            if (m_gsm_hw_buffer.at_cmd.send_at_cb != NULL)
            {
                m_gsm_hw_buffer.at_cmd.send_at_cb(GSM_EVEN_OK, m_gsm_hw_buffer.at_cmd.recv_buffer.buffer);
            }
        }

        /* A connection has been established; the DCE is moving from command state to online data state */
        if (m_is_new_serial_data)
        {
            if (strstr((char *)(m_gsm_hw_buffer.at_cmd.recv_buffer.buffer), AT_CONNTECT))
            {
                DebugPrint("Modem connected\r\n");
            }
            m_is_new_serial_data = false;
        }
    }
    else
    {
        if (m_is_new_serial_data)
        {
            /* The connection has been terminated or the attempt to establish a connection failed */
            if (strstr((char *)(m_gsm_hw_buffer.recv_buffer.buffer), GSM_NO_CARRIER))
            {
                DebugPrint("Modem connection lost. Reconnect now\r\n");
                memset(m_gsm_hw_buffer.recv_buffer.buffer, 0, GSM_PPP_MODEM_BUFFER_SIZE);
                if (gsm_manager_ctx()->ppp_phase != PPP_PHASE_RUNNING)
                {
                    gsm_manager_ctx()->ppp_phase = PPP_PHASE_DEAD;
                    ppp_close(gsm_data_layer_get_ppp_control_block(), 1);
                }
                else
                {
                    gsm_manager_ctx()->state = GSM_REOPEN_PPP;
                    gsm_manager_ctx()->step = 0;
                }
            }
        }
    }
}

void gsm_hw_send_at_cmd(char *cmd, char *expect_response, uint16_t timeout_ms,
                        uint8_t retry_cnt, gsm_send_AT_cb_t callback)
{
    if (timeout_ms == 0 || callback == NULL)
    {
        m_gsm_hw_config.serial_tx((uint8_t *)cmd, strlen(cmd));
        return;
    }

    m_gsm_hw_buffer.at_cmd.cmd = cmd;
    m_gsm_hw_buffer.at_cmd.expect_response_from_atc = expect_response;
    m_gsm_hw_buffer.at_cmd.recv_buffer.buffer_idx = 0;
    m_gsm_hw_buffer.at_cmd.recv_buffer.state = BUFFER_STATE_IDLE;
    m_gsm_hw_buffer.at_cmd.retry_cnt_atc = retry_cnt;
    m_gsm_hw_buffer.at_cmd.send_at_cb = callback;
    m_gsm_hw_buffer.at_cmd.timeout_atc = timeout_ms / gsm_hw_get_config()->hw_polling_ms; //gsm_hw_polling_task: 10ms /10, 100ms /100
    m_gsm_hw_buffer.at_cmd.current_timeout_atc = 0;

    memset(m_gsm_hw_buffer.at_cmd.recv_buffer.buffer, 0, sizeof(m_gsm_hw_buffer.at_cmd.recv_buffer.buffer));
    m_gsm_hw_buffer.at_cmd.recv_buffer.buffer_idx = 0;
    m_gsm_hw_buffer.at_cmd.recv_buffer.state = BUFFER_STATE_IDLE;

    m_gsm_hw_config.serial_tx((uint8_t *)cmd, strlen(cmd));

    DebugPrint("\r\r\n%s\r\n", cmd);
}

uint32_t sio_read(sio_fd_t fd, u8_t *data, u32_t len)
{
    uint32_t i = 0;

    for (i = 0; i < len; i++)
    {
        if (m_gsm_hw_buffer.recv_buffer.idx_out == m_gsm_hw_buffer.recv_buffer.idx_in)
        {
            return i;
        }

        data[i] = m_gsm_hw_buffer.recv_buffer.buffer[m_gsm_hw_buffer.recv_buffer.idx_out];

        m_gsm_hw_buffer.recv_buffer.idx_out++;
        if (m_gsm_hw_buffer.recv_buffer.idx_out == GSM_PPP_MODEM_BUFFER_SIZE)
            m_gsm_hw_buffer.recv_buffer.idx_out = 0;
    }

    return i;
}

gsm_hw_config_t *gsm_hw_get_config(void)
{
    return &m_gsm_hw_config;
}

void gsm_hw_lwip_polling_task(void)
{
    uint32_t sio_size;

    sys_check_timeouts();

    sio_size = sio_read(0, m_ppp_rx_buffer, sizeof(m_ppp_rx_buffer));
    if (sio_size > 0)
    {
        pppos_input(gsm_data_layer_get_ppp_control_block(), m_ppp_rx_buffer, sio_size);
    }
}

void gsm_hw_hardreset(gsm_power_on_cb_t cb)
{
    static gsm_power_on_cb_t m_cb = NULL;
    static uint8_t step = 0;
    static uint32_t last_reset = 0;
    static bool m_last_power_state_is_up = false;

    m_cb = cb;

    if (m_gsm_hw_config.sys_now() - last_reset > 1000)
    {
        last_reset = m_gsm_hw_config.sys_now();
    }
    else
    {
        return;
    }

    if (step == 0)
    {
        if (m_gsm_hw_config.io_get(m_gsm_hw_config.gpio.status_pin))
        {
            DebugPrint("GSM previous power state is up\r\n");
            m_last_power_state_is_up = true;
        }
        else
        {
            DebugPrint("GSM previous power state is down\r\n");
            m_last_power_state_is_up = false;
        }
    }

    if (m_last_power_state_is_up == false) // Power on module
    {
        switch (step)
        {
        case 0:
            m_gsm_hw_config.uart_initialize();
            m_gsm_hw_config.io_set(m_gsm_hw_config.gpio.power_key, 0);
            step++;
            break;

        case 1:
            step++;
            break;

        case 2:
            step++;
            break;

        case 3:
            m_gsm_hw_config.io_set(m_gsm_hw_config.gpio.power_key, 0);
            step++;
            break;

        case 4:
            step++;
            break;

        case 5:
            step++;
            break;

        case 6: //Status Low after 2s -> PowerKey Low to power ON
            m_gsm_hw_config.io_set(m_gsm_hw_config.gpio.power_key, 1);
            step++;
            break;

        case 7:
            step++;
            break;
        case 8:
            step++;
            break;

        case 9: //Powerkey High
            m_gsm_hw_config.io_set(m_gsm_hw_config.gpio.power_key, 0);
            step = 0;

            if (m_gsm_hw_config.io_get(m_gsm_hw_config.gpio.status_pin))
            {
                DebugPrint("GSM power ready\r\n");
                if (m_cb)
                    m_cb();
                m_last_power_state_is_up = true;
            }
            else
            {
                DebugPrint("GSM power on failed\r\n");
                m_last_power_state_is_up = false;
            }
            break;

        default:
            // Shoud never get there
            break;
        }
    }
    else // Power off module
    {
        switch (step)
        {
        case 0:
            DebugPrint("GSM power off\r\n");
            m_gsm_hw_config.uart_initialize();
            m_gsm_hw_config.io_set(m_gsm_hw_config.gpio.power_key, 1);
            step++;
            break;

        case 1:
            step++;
            break;

        case 2:
            step++;
            break;

        case 3:
            m_last_power_state_is_up = false;
            step = 0;
            GSM_CLEAR_BUFFER();
            break;

        default:
            // Shoud never get there
            break;
        }
    }
}
