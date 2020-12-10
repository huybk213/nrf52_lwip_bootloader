#include <stdio.h>
#include <string.h>
#include "lwip/tcp.h"
#include "lwip/tcpip.h"
#include "lwip/apps/http_client.h"
#include "netif/ppp/polarssl/md5.h"
#include "app_bootloader_define.h"
#include "nrf_nvmc.h"

#define MD5_LEN 16
#define HEADER_SIZE 16

static httpc_connection_t m_conn_settings_try;
static uint8_t m_last_download_percent;
static uint8_t m_firmware_signature[HEADER_SIZE] = { 0x48, 0x55, 0x59, 0x5F, 0x54, 0x56, 0x5F, 0x54, 0x45, 0x53, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00 };
static httpc_state_t *m_http_connection_state;

static bool update_fw_verify_checksum(uint32_t begin_addr, uint32_t size)
{
    DebugPrint("Caculate md5 from addr 0x%08X, size %d\r\n", begin_addr, size - MD5_LEN);
	md5_context	md5_ctx;
	uint8_t md5_buffer[MD5_LEN];

	md5_starts(&md5_ctx);
	md5_update(&md5_ctx, (uint8_t*)begin_addr, size - MD5_LEN);
	md5_finish(&md5_ctx, md5_buffer);

	uint32_t checksum_addr = begin_addr + size - MD5_LEN;
    uint8_t * md5_test = (uint8_t*)checksum_addr;
	if (memcmp(md5_buffer, (uint8_t*)checksum_addr, MD5_LEN) == 0)
    {
        DebugPrint("Valid md5\r\n");
		return true;
    }
	else
    {
       	DebugPrint("Calculated %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X, expected %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n", 
                   md5_buffer[0], md5_buffer[1], md5_buffer[2], md5_buffer[3], md5_buffer[4], md5_buffer[5], md5_buffer[6], md5_buffer[7],
                   md5_buffer[8], md5_buffer[9], md5_buffer[10], md5_buffer[11], md5_buffer[12], md5_buffer[13], md5_buffer[14], md5_buffer[15],
                   md5_test, md5_test[1], md5_test[2], md5_test[3], md5_test[4], md5_test[5], md5_test[6], md5_test[7],
                   md5_test[8], md5_test[9], md5_test[10], md5_test[11], md5_test[12], md5_test[13], md5_test[14], md5_test[15]);
		return false;
    }
}

/** Handle data connection incoming data
 * @param pointer to lwftp session data
 * @param pointer to PCB
 * @param pointer to incoming pbuf
 * @param state of incoming process
 */
static err_t httpc_file_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{		
  	if (p) 
	{	  
		struct pbuf *q;
		for (q=p; q; q=q->next) 
		{
			static bool verify_signature = false;
			uint8_t * payload = (uint8_t*)q->payload;
			uint32_t write_size = q->len;
			if (verify_signature == false)
			{
				verify_signature = true;
				// Verify header
				if (memcmp(m_firmware_signature, payload, strlen((char*)m_firmware_signature)))
				{
					DebugPrint("Invalid header\r\n");
					app_bootloader_ota_finish("Invalid header");
					NVIC_SystemReset();
				}                    
	//                DebugPrint("\rFirst 16 bytes %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n", 
	//                        payload[0], payload[1], payload[2], payload[3], 
	//                        payload[4], payload[5], payload[6], payload[7],  
	//                        payload[8], payload[9], payload[10], payload[11],
	//                        payload[12], payload[13], payload[14], payload[15]);
				// We dont need to check write size > 16
				payload += HEADER_SIZE;
				write_size -= HEADER_SIZE;
				uint32_t address = app_bootloader_get_download_address();
				
				DebugPrint("Erase data at addr 0x%08X to addr 0x%08X\r\n", address, address + APPLICATION_MAX_FILE_SIZE);
				for (uint32_t i = 0; i < APPLICATION_MAX_FILE_SIZE/4096; i++)
				{
					nrf_nvmc_page_erase(address);
					address += 4096;
				}
			}

		if (write_size > 0)
		{	
			nrf_nvmc_write_bytes(app_bootloader_get_download_address() + gsm_ctx()->file_transfer.size_downloaded, payload, write_size);
		}

	//            DebugPrint("\rHTTP written: %u bytes, current addr 0x%08X, wr size %d\r\n", 
	//                        gsm_ctx()->file_transfer.DataTransfered, 
	//                        APPLICATION_START_ADDDR + gsm_ctx()->file_transfer.DataTransfered,
	//                        write_size);
		
			gsm_ctx()->file_transfer.state = FT_TRANSFERRING;
			gsm_ctx()->file_transfer.size_downloaded += write_size;
		}
	  
		// MUST used 2 commands!
		tcp_recved(tpcb, p->tot_len);
		pbuf_free(p);
	  
		if (gsm_ctx()->file_transfer.size_downloaded >= gsm_ctx()->file_transfer.size.value)
		{
			DebugPrint("HTTP get file DONE. Transfers: %u\r\n", gsm_ctx()->file_transfer.size_downloaded);
			if (update_fw_verify_checksum(app_bootloader_get_download_address(), gsm_ctx()->file_transfer.size_downloaded))
			{
                app_bootloader_update_ota_address();
                DebugPrint("OTA success\r\n");
				app_bootloader_ota_finish("Success");
			}
			else
			{
				DebugPrint("Invalid firmware\r\n");
                app_bootloader_ota_finish("Signature");
			}
            
            DebugFlush();
			NVIC_SystemReset();
							
			tcp_close(tpcb);
			return ERR_ABRT;
		}
	} 
	else 
	{
		DebugPrint("tcp_close\r\n");
	// NULL pbuf shall lead to close the pcb. Close is postponed after
	// the session state machine updates. No need to close right here.
	// Instead we kindly tell data sink we are done
		tcp_close(tpcb);
		return ERR_ABRT;
	}
	
	return ERR_OK;
}

/**
 * @brief Result transfer done callback
 */
static void httpc_result_callback(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err)
{
	DebugPrint("result: %d, content len: %d, status code: %d\r\n", httpc_result, rx_content_len, srv_res);
    if (rx_content_len == 0)
    {
        app_bootloader_ota_finish("Length");
        DebugFlush();
        NVIC_SystemReset();
    }
	
	/* Xu ly loi connection */
	switch(err)
	{
		case HTTPC_RESULT_OK:		/** File successfully received */
            if (rx_content_len == gsm_ctx()->file_transfer.size.value)
            {
				if (update_fw_verify_checksum(app_bootloader_get_download_address(), gsm_ctx()->file_transfer.size_downloaded))
				{
                    app_bootloader_update_ota_address();
					app_bootloader_ota_finish("Success");
				}
				else
				{
					DebugPrint("Invalid firmware\r\n");		
				}
				NVIC_SystemReset();   
            }
         
//			ProcessDownloadDone(rx_content_len);
			break;

		case HTTPC_RESULT_ERR_UNKNOWN:		 /** Unknown error */
			//break;
		case HTTPC_RESULT_ERR_CONNECT:		/** Connection to server failed */
			//break;
		case HTTPC_RESULT_ERR_HOSTNAME:	/** Failed to resolve server hostname */
			//break;
		case HTTPC_RESULT_ERR_CLOSED:		/** Connection unexpectedly closed by remote server */
			//break;
		case HTTPC_RESULT_ERR_TIMEOUT:		 /** Connection timed out (server didn't respond in time) */
			//break;
		case HTTPC_RESULT_ERR_SVR_RESP:		/** Server responded with an error code */
			//break;
		case HTTPC_RESULT_ERR_MEM:		/** Local memory error */
			//break;
		case HTTPC_RESULT_LOCAL_ABORT:		/** Local abort */
			//break;
		case HTTPC_RESULT_ERR_CONTENT_LEN:		/** Content length mismatch */
            app_bootloader_update_ota_address();
            app_bootloader_ota_finish("Length");
            DebugPrint("Error content length\r\n");
            DebugFlush();
			NVIC_SystemReset();
			break;
		default:
			break;
	}
}

/**
 * @brief Header received done callback
 */
static err_t httpc_headers_done_callback (httpc_state_t *connection, void *arg, struct pbuf *hdr, u16_t hdr_len, u32_t content_len)
{
//	DebugPrint("\r\r\thttpc_headers_callback %s\r\n", (char*)hdr->payload);
    if (content_len > APPLICATION_MAX_FILE_SIZE + + HEADER_SIZE)
    {
        DebugPrint("Firmware size is too long [%d], maximum %d\r\n", content_len, APPLICATION_MAX_FILE_SIZE + HEADER_SIZE);
        app_bootloader_ota_finish("File size");
        NVIC_SystemReset();
    }        
	
    gsm_ctx()->file_transfer.size.value = content_len;
	gsm_ctx()->file_transfer.size_downloaded = 0;

	return ERR_OK;
}

void update_fw_http_polling_task(void)
{
	if (gsm_ctx()->file_transfer.state == FT_WAIT_TRANSFER_STATE)
	{
        DebugPrint("Begin http download %s%s port %d\r\n", gsm_ctx()->file_transfer.server, 
                                                           gsm_ctx()->file_transfer.file, 
                                                           gsm_ctx()->file_transfer.port);

		gsm_ctx()->file_transfer.size_downloaded = 0;
		
//		err_t error = httpc_get_file(&server_addr, gsm_ctx()->file_transfer.FTP_Port, file_url, &m_conn_settings_try, httpc_file_recv_callback, NULL, NULL);
		err_t error = httpc_get_file_dns((const char*)gsm_ctx()->file_transfer.server, 
                                        gsm_ctx()->file_transfer.port, 
                                        (const char*)gsm_ctx()->file_transfer.file, 
                                        &m_conn_settings_try, httpc_file_recv_callback, 
                                        NULL, 
                                        &m_http_connection_state);
		
//		DebugPrint("\r\thttpc_get_file: %d", error);
		
		if (error != ERR_OK)
		{
			DebugPrint("\rCannot connect HTTP server");
			gsm_ctx()->file_transfer.state = FT_NO_TRANSER;
            NVIC_SystemReset();
		}
		else
		{
//			DebugPrint("\rDownload file %s\r\n", gsm_ctx()->file_transfer.HTTP_File);
			gsm_ctx()->file_transfer.state = FT_TRANSFERRING;
		}
	}
}


void update_fw_polling_download_status(void) 
{  	
	//Neu dang download -> cap nhat tien do download len mqtt
	if (gsm_ctx()->file_transfer.state == FT_TRANSFERRING &&
		gsm_ctx()->file_transfer.size_downloaded  > 0 && 
        gsm_ctx()->file_transfer.size.value > 0)
	{
		
		uint8_t cur_percent = gsm_ctx()->file_transfer.size_downloaded*100/gsm_ctx()->file_transfer.size.value;
		if ((cur_percent - m_last_download_percent > 5) || cur_percent == 100)
		{
			m_last_download_percent = cur_percent;
			DebugPrint("Percent %d\r\n", m_last_download_percent);
		}
	}
}


void update_fw_initialize(void) 
{  
	/* Init Http connection params */
	m_conn_settings_try.use_proxy = 0;
	m_conn_settings_try.headers_done_fn = httpc_headers_done_callback;
	m_conn_settings_try.result_fn = httpc_result_callback;
    
    gsm_ctx()->file_transfer.state = FT_NO_TRANSER;	
}



