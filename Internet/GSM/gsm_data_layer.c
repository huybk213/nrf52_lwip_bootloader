#include "gsm.h"
#include "gsm_context.h"
#include "gsm_utilities.h"
#include "string.h"
#include "ppp.h"
#include "pppos.h"
#include "lwip/init.h"
#include "dns.h"
#include "gsm_utilities.h"

static volatile gsm_manager_t m_gsm_manager;
static volatile int m_ppp_connected = false;

/*
 * Globals
 * =======
 */
/* The PPP control block */
static ppp_pcb *m_ppp_control_block;

/* The PPP IP interface */
static struct netif ppp_netif;


static void gsm_power_on(gsm_response_evt_t event, void *response_buffer);

static void change_gsm_state(gsm_state_t state);

static void open_ppp_stack(gsm_response_evt_t event, void *response_buffer);


ppp_pcb * gsm_data_layer_get_ppp_control_block(void)
{
	return m_ppp_control_block;
}

/**
 * PPP status callback
 * ===================
 *
 * PPP status callback is called on PPP status change (up, down, ...) from lwIP core thread
 */
static void ppp_link_status_cb(ppp_pcb *pcb, int err_code, void *ctx)
{
	struct netif *pppif = ppp_netif(pcb);
	LWIP_UNUSED_ARG(ctx);

	switch (err_code)
	{
		case PPPERR_NONE:
		{
			#if LWIP_DNS
			//		const ip_addr_t *ns;
			#endif /* LWIP_DNS */

            DebugPrint("PPP Connected\r\n");

			#if PPP_IPV4_SUPPORT

            DebugPrint("our_ipaddr  = %s\r\n", ipaddr_ntoa(&pppif->ip_addr));
            DebugPrint("his_ipaddr  = %s\r\n", ipaddr_ntoa(&pppif->gw));
            DebugPrint("netmask    = %s\r\n", ipaddr_ntoa(&pppif->netmask));

			#if LWIP_DNS
			//		ns = dns_getserver(0);
			//		DebugPrint("\tdns1        = %s\r\n", ipaddr_ntoa(ns));
			//		ns = dns_getserver(1);
			//		DebugPrint("\tdns2        = %s\r\n", ipaddr_ntoa(ns));
			#endif /* LWIP_DNS */

			#endif /* PPP_IPV4_SUPPORT */

			#if PPP_IPV6_SUPPORT
					DebugPrint("\r   our6_ipaddr = %s\n", ip6addr_ntoa(netif_ip6_addr(pppif, 0)));
			#endif /* PPP_IPV6_SUPPORT */
			break;
		}

		case PPPERR_PARAM:
		{
			DebugPrint("status_cb: Invalid parameter\r\n");
			break;
		}
		case PPPERR_OPEN:
		{
			DebugPrint("status_cb: Unable to open PPP session\r\n");
			break;
		}
		case PPPERR_DEVICE:
		{
			DebugPrint("status_cb: Invalid I/O device for PPP\r\n");
			break;
		}
		case PPPERR_ALLOC:
		{
			DebugPrint("status_cb: Unable to allocate resources\r\n");
			break;
		}
		case PPPERR_USER: /* 5 */
		{
			/* ppp_close() was previously called, reconnect */
			DebugPrint("status_cb: ppp is closed by user OK! Try to re-open...\r\n");
			/* ppp_free(); -- can be called here */
			ppp_free(m_ppp_control_block);
			change_gsm_state(GSM_REOPEN_PPP);        
			break;
		}
		case PPPERR_CONNECT: /* 6 */
		{
			DebugPrint("status_cb: Connection lost\r\n");
			m_ppp_connected = false;
			ppp_close(m_ppp_control_block, 1);
			break;
		}
		case PPPERR_AUTHFAIL:
		{
			DebugPrint("status_cb: Failed authentication challenge\r\n");
			break;
		}
		case PPPERR_PROTOCOL:
		{
			DebugPrint("status_cb: Failed to meet protocol\n");
			break;
		}
		case PPPERR_PEERDEAD:
		{
			DebugPrint("status_cb: Connection timeout\r\n");
			break;
		}
		case PPPERR_IDLETIMEOUT:
		{
			DebugPrint("status_cb: Idle Timeout\r\n");
			break;
		}
		case PPPERR_CONNECTTIME:
		{
			DebugPrint("status_cb: Max connect time reached\r\n");
			break;
		}
		case PPPERR_LOOPBACK:
		{
			DebugPrint("status_cb: Loopback detected\r\n");
			break;
		}
		default:
		{
			DebugPrint("status_cb: Unknown error code %d\r\n", err_code);
			break;
		}
	}

	/*
	* This should be in the switch case, this is put outside of the switch
	* case for example readability.
	*/

	if (err_code == PPPERR_NONE)
	{
		DebugPrint("PPP is opened OK\r\n!");
		return;
	}

	//  /* ppp_close() was previously called, don't reconnect */
	//  if (err_code == PPPERR_USER) {
	//    /* ppp_free(); -- can be called here */
	//	 m_ppp_connected = false;
	//	 ppp_free(m_ppp_control_block);
	//	 DebugPrint("\r PPP opened ERR!");
	//    return;
	//  }

	/*
   * Try to reconnect in 30 seconds, if you need a modem chatscript you have
   * to do a much better signaling here ;-)
   */
	//  ppp_connect(pcb, 30);
	/* OR ppp_listen(pcb); */
}

static void ppp_notify_phase_cb(ppp_pcb *pcb, u8_t phase, void *ctx)
{
	switch (phase)
	{
		/* Session is down (either permanently or briefly) */
		case PPP_PHASE_DEAD:
			DebugPrint("PPP_PHASE_DEAD\r\n");
			m_gsm_manager.ppp_phase = PPP_PHASE_DEAD;
			break;

		/* We are between two sessions */
		case PPP_PHASE_HOLDOFF:
			DebugPrint("PPP_PHASE_HOLDOFF\r\n");
			m_gsm_manager.ppp_phase = PPP_PHASE_HOLDOFF;
			break;

		/* Session just started */
		case PPP_PHASE_INITIALIZE:
			DebugPrint("PPP_PHASE_INITIALIZE\r\n");
			m_gsm_manager.ppp_phase = PPP_PHASE_INITIALIZE;
			break;
        
        case PPP_PHASE_NETWORK:
            DebugPrint("PPP_PHASE_NETWORK\r\n");
            break;
        
		/* Session is running */
		case PPP_PHASE_RUNNING:
			DebugPrint("PPP_PHASE_RUNNING\r\n");
			m_gsm_manager.ppp_phase = PPP_PHASE_RUNNING;
			m_ppp_connected = true;
			break;
        
        case PPP_PHASE_TERMINATE:
            DebugPrint("PPP_PHASE_TERMINATE\r\n");
            break;
        
        case PPP_PHASE_DISCONNECT:
            DebugPrint("PPP_PHASE_DISCONNECT\r\n");
            break;
        
		default:
            DebugPrint("PPP phase %d\r\n", phase);
			break;
	}
}

/*
 * PPPoS serial output callback
 *
 * ppp_pcb, PPP control block
 * data, buffer to write to serial port
 * len, length of the data buffer
 * ctx, optional user-provided callback context pointer
 *
 * Return value: len if write succeed
 */
static u32_t ppp_output_callback(ppp_pcb *pcb, u8_t *data, u32_t len, void *ctx)
{
	uint32_t tx_len = gsm_hw_get_config()->serial_tx(data, len);
	return tx_len;
}

void gsm_power_on_cb(void)
{
	change_gsm_state(GSM_STATE_PWR_ON);
}

void gsm_state_polling_task(void)
{
    static uint32_t last_tick = 0;
    if (gsm_hw_get_config()->sys_now() - last_tick > 1000)
    {
        last_tick = gsm_hw_get_config()->sys_now();
    }
    else
    {
        return;
    }

	switch (m_gsm_manager.state)
	{
		case GSM_STATE_PWR_ON:
			if (m_gsm_manager.step == 0)
			{
				m_gsm_manager.step = 1;
				gsm_hw_send_at_cmd(ATV1, AT_OK, 1000, 10, gsm_power_on);
			}
			break;

		case GSM_OK:
			break;

		case GSM_STATE_RESET: /* Hard Reset */
				m_gsm_manager.ready = 0;
				m_gsm_manager.mode = GSM_AT_MODE;
				gsm_hw_hardreset(gsm_power_on_cb);
				break;

		case GSM_STATE_CLOSE_PPP:
				if (m_gsm_manager.step == 0)
				{
					DebugPrint("Close PPP\r\n");
					m_gsm_manager.step = 1;
					m_ppp_connected = false;
					ppp_close(m_ppp_control_block, 1);
				}
				break;

		case GSM_REOPEN_PPP:
				if (m_gsm_manager.step == 0)
				{
					m_gsm_manager.step = 1;
					m_ppp_connected = false;
					gsm_hw_send_at_cmd(ATV1, AT_OK, 1000, 3, open_ppp_stack);
				}
			break;

		default:
			break;
	}
}


void gsm_data_layer_initialize(void)
{
	change_gsm_state(GSM_STATE_RESET);
}

static void change_gsm_state(gsm_state_t state)
{
	m_gsm_manager.state = state;
	m_gsm_manager.step = 0;

	if (state == GSM_OK)
	{
		m_gsm_manager.mode = GSM_PPP_MODE;
		gsm_hw_send_at_cmd(ATO, AT_CONNTECT, 1000, 10, NULL);
	}
	else
	{
		m_gsm_manager.mode = GSM_AT_MODE;
	}

	DebugPrint("Change GSM state to : %u\r\n", state);
}

static void gsm_power_on(gsm_response_evt_t event, void *response_buffer)
{
	switch (m_gsm_manager.step)
	{
		case 1:
		{
			if (event != GSM_EVEN_OK)
			{
				DebugPrint("Connect to module GSM failed\r\n");
			}
			gsm_hw_send_at_cmd(AT_ECHO_OFF, AT_OK, 1000, 10, gsm_power_on);
		}
			break;

		case 2:
		{
			DebugPrint("Setup echo mode off: %s", (event == GSM_EVEN_OK) ? "[OK]" : "[FAIL]");
			gsm_hw_send_at_cmd(AT_BAUDRATE_115200, AT_OK, 1000, 10, gsm_power_on);
		}
			break;

		case 3:
		{
			DebugPrint("Setup communication speed: %s\r\n", (event == GSM_EVEN_OK) ? "[OK]" : "[FAIL]");
			gsm_hw_send_at_cmd(AT_REPORT_EQUIPMENT_ERROR_ENABLE, AT_OK, 1000, 10, gsm_power_on);
		}
			break;

		case 4:
		{
			DebugPrint("Error notification enable: %s\r\n", (event == GSM_EVEN_OK) ? "[OK]" : "[FAIL]");
			gsm_hw_send_at_cmd(AT_NEW_MESSSAGE_INDICATION, AT_OK, 1000, 10, gsm_power_on);
		}
			break;
		case 5:
			DebugPrint("Enable SMS receiver: %s\r\n", (event == GSM_EVEN_OK) ? "[OK]" : "[FAIL]");
			gsm_hw_send_at_cmd(AT_CPIN, AT_OK, 1000, 5, gsm_power_on);
			break;

		case 6:
		{
			DebugPrint("Get SIM state: %s\r\n", (event == GSM_EVEN_OK) ? "[OK]" : "[FAIL]");
			if (strstr(response_buffer, GSM_SIM_NOT_INSERT))
			{
				DebugPrint("Sim card not inserted\r\n");
				change_gsm_state(GSM_STATE_RESET);
				return;
			}
			else
			{
				gsm_hw_send_at_cmd(AT_CIMI, AT_OK, 1000, 10, gsm_power_on);
			}
		}
			break;

		case 7:
		{
			memset(gsm_ctx()->parameters.sim_imei, 0, sizeof(gsm_ctx()->parameters.sim_imei));
			gsm_utilities_get_imei(response_buffer, (uint8_t*)gsm_ctx()->parameters.sim_imei);
	//		DebugPrint("Get SIM IMEI: %s\r\n", gsm_ctx()->parameters.sim_imei);
			if (strlen(gsm_ctx()->parameters.sim_imei) < 15)
			{
				DebugPrint("GSM: invalid SIM IMEI %s\r\n", gsm_ctx()->parameters.sim_imei);
				change_gsm_state(GSM_STATE_RESET);
				return;
			}
			gsm_hw_send_at_cmd(AT_GSN, AT_OK, 3000, 10, gsm_power_on);
		}
			break;

		case 8:
		{
			memset(gsm_ctx()->parameters.gsm_imei, 0, sizeof(gsm_ctx()->parameters.gsm_imei));
			gsm_utilities_get_imei(response_buffer, (uint8_t*)gsm_ctx()->parameters.gsm_imei);
			DebugPrint("Get GSM IMEI: %s\r\n", gsm_ctx()->parameters.gsm_imei);
			if (strlen(gsm_ctx()->parameters.gsm_imei) < 15)
			{
				DebugPrint("GSM: Invalid GSM IMEI\r\n");
				change_gsm_state(GSM_STATE_RESET);
				return;
			}
			gsm_hw_send_at_cmd(AT_CFUN, AT_OK, 3000, 3, gsm_power_on);
		}
			break;

		case 9:
		{
			DebugPrint("Set ME Functionality: %s\r\n", (event == GSM_EVEN_OK) ? "[OK]" : "[FAIL]");
			gsm_hw_send_at_cmd(AT_CMGF, AT_OK, 1000, 10, gsm_power_on);
		}
			break;

		case 10:
		{
			DebugPrint("Setup SMS in text mode: %s\r\n", (event == GSM_EVEN_OK) ? "[OK]" : "[FAIL]");
			gsm_hw_send_at_cmd(AT_SETUP_APN, AT_OK, 1000, 2, gsm_power_on);
		}
			break;

		case 11:
		{
			DebugPrint("Setup APN : %s\r\n", (event == GSM_EVEN_OK) ? "[OK]" : "[FAIL]");
			gsm_hw_send_at_cmd(AT_CGREG, AT_OK, 1000, 3, gsm_power_on);
		}
			break;
		
		case 12:
		{
			DebugPrint("Register GPRS network: %s\r\n", (event == GSM_EVEN_OK) ? "[OK]" : "[FAIL]");
			gsm_hw_send_at_cmd(AT_CSQ, AT_OK, 1000, 5, gsm_power_on);
		}
			break;

		case 13:
		{
			if (event != GSM_EVEN_OK)
			{
				DebugPrint("GSM: Cannot initialize module. Reset now\r\n");
				change_gsm_state(GSM_STATE_RESET);
				return;
			}

			if (gsm_utilities_get_signal_strength_from_buffer(response_buffer, &gsm_ctx()->gl_status.gsm_csq) == false)
			{
				gsm_ctx()->gl_status.gsm_csq = GSM_CSQ_INVALID;
			}

			if (gsm_ctx()->gl_status.gsm_csq == GSM_CSQ_INVALID || gsm_ctx()->gl_status.gsm_csq == 0)
			{
				gsm_hw_send_at_cmd(AT_CSQ, AT_OK, 1000, 3, gsm_power_on);
				m_gsm_manager.step = 12;
			}
			else
			{
				DebugPrint("Valid CSQ: %d\r\n", gsm_ctx()->gl_status.gsm_csq);
				m_gsm_manager.step = 0;
				gsm_manager_ctx()->ready = 1;
				gsm_hw_send_at_cmd(ATV1, AT_OK, 1000, 5, open_ppp_stack);
			}
		}
			break;

		default:
			DebugPrint("Unknown step %d\r\n", m_gsm_manager.step);
			break;
	}

	m_gsm_manager.step++;
}


bool gsm_data_layer_is_ppp_connected(void)
{
    return m_ppp_connected;
}

static void open_ppp_stack(gsm_response_evt_t event, void *response_buffer)
{
    DebugPrint("Open PPP stack\r\n");
    static uint8_t retry_count = 0;

    switch (m_gsm_manager.step)
    {
		case 1:
		{
			gsm_hw_send_at_cmd(AT_CSQ, AT_OK, 1000, 2, open_ppp_stack);
			m_gsm_manager.step = 2;
		}
			break;

    	case 2:
		{
			//Check SIM inserted, if removed -> RESET module NOW!
			gsm_hw_send_at_cmd(AT_CPIN, AT_OK, 1000, 5, open_ppp_stack);
			m_gsm_manager.step = 3;
		}
			break;

    	case 3:
		{
        	if (event == GSM_EVEN_OK)
          	{
				if (strstr(response_buffer, GSM_SIM_NOT_INSERT))
				{
					DebugPrint("Sim card not inserted\r\n");
					// Reset gsm module
					change_gsm_state(GSM_STATE_RESET);
					return;
				}
				gsm_hw_send_at_cmd(AT_ENTER_PPP, AT_CONNTECT, 1000, 10, open_ppp_stack);
				m_gsm_manager.step = 4;
          	} 
			else 
			{
            	// Reset gsm module
            	change_gsm_state(GSM_STATE_RESET);
            	return;
         	}
		}
          	break;
    	case 4:
		{
        	DebugPrint("PPP state: %s\r\n", (event == GSM_EVEN_OK) ? "[OK]" : "[FAIL]");
        	m_gsm_manager.mode = GSM_PPP_MODE;

        	if (event != GSM_EVEN_OK)
          	{
                retry_count++;
                if (retry_count > 4)
                {
					retry_count = 0;
					ppp_close(m_ppp_control_block, 0);

					/* Reset GSM */
					change_gsm_state(GSM_STATE_RESET);
                }
                else
                {
                    m_gsm_manager.step = 3;
                    ppp_close(m_ppp_control_block, 0);
                    gsm_hw_send_at_cmd(ATV1, AT_OK, 1000, 5, open_ppp_stack);
                }
          	}
          	else
          	{
			    change_gsm_state(GSM_OK);
			    
				//Create PPP connection
			    m_ppp_control_block = pppos_create(&ppp_netif, ppp_output_callback, ppp_link_status_cb, NULL);
			    if (m_ppp_control_block == NULL)
			    {
					DebugPrint("Create PPP interface ERR!\r\n");
				}

			    /* Set this interface as default route */
			    ppp_set_default(m_ppp_control_block);
			    ppp_set_auth(m_ppp_control_block, PPPAUTHTYPE_CHAP, "", "");
			    ppp_set_notify_phase_callback(m_ppp_control_block, ppp_notify_phase_cb);
			    ppp_connect(m_ppp_control_block, 0);
          	}
		}
        	break;
    }
}

gsm_manager_t * gsm_manager_ctx(void)
{
    return (gsm_manager_t*)&m_gsm_manager;
}

void gsm_set_at_mode(gsm_at_mode_t mode)
{
	m_gsm_manager.mode = mode;
}
