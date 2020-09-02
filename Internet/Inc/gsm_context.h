#ifndef SYSTEM_CONTEXT_H
#define SYSTEM_CONTEXT_H

#include "SEGGER_RTT.h"
#include "app_bootloader_define.h"

#define	DebugPrint(String...)	SEGGER_RTT_printf(0, String)
//#define	DebugPrint(String...)	while(0)

#define	BUFFER_STATE_BUSY   		1       // Trang thai dang them du lieu
#define	BUFFER_STATE_IDLE   		2       // Trang thai cho
#define	BUFFER_STATE_PROCESSING   	3 		// Trang thai du lieu dang duoc xu ly

#define	FTP_STATE_OPENDED   1
#define	FTP_STATE_CLOSED    2

#define	LARGE_BUFFER_SIZE   1000
#define	SMALL_BUFFER_SIZE   200

#define GSM_IMEI_MAX_LENGTH (16)
#define GSM_CSQ_INVALID     (99)

typedef union
{
    float value;
    uint8_t bytes[4];
} Float_t;

typedef union
{
    uint32_t value;
    uint8_t bytes[4];
} Long_t;

typedef union
{
    uint16_t value;
    uint8_t bytes[2];
} Int_t;

typedef struct
{
    uint8_t Buffer[LARGE_BUFFER_SIZE];
    uint16_t BufferIndex;
    uint8_t State;
} LargeBuffer_t;

typedef struct
{
    uint8_t Buffer[SMALL_BUFFER_SIZE];
    uint16_t BufferIndex;
    uint8_t State;
} SmallBuffer_t;

typedef enum
{
	FT_NO_TRANSER = 0,
	FT_REQUEST_SRV_RESPONSE,
	FT_WAIT_TRANSFER_STATE,
	FT_TRANSFERRING,
	FT_WAIT_RESPONSE,
	FT_TRANSFER_DONE
} file_transfer_state_t;

typedef struct 
{
    uint8_t server[HTTP_MAX_FILE_SIZE];
    uint8_t file[HTTP_MAX_FILE_SIZE];
    uint16_t port;
    Long_t size;
    uint32_t size_downloaded;
    file_transfer_state_t state;
} gsm_ctx_file_transfer_t;

typedef struct
{
	uint8_t ConnectOK;
    uint8_t GSMCSQ;
} gsm_ctx_status_t;	


typedef struct
{
    char GSM_IMEI[GSM_IMEI_MAX_LENGTH];
    char SIM_IMEI[GSM_IMEI_MAX_LENGTH];
} gsm_ctx_param_t;

typedef struct
{
	gsm_ctx_status_t gl_status;	
	gsm_ctx_file_transfer_t file_transfer;
    gsm_ctx_param_t Parameters;
} gsm_ctx_t;

/**
 * @brief Get gsm context
 * @retval Pointer to gsm context 
 */
gsm_ctx_t * gsm_ctx(void);

#endif // SYSTEM_CONTEXT_H


