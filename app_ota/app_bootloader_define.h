

#ifndef APP_BOOTLOADER_DEFINE_H
#define APP_BOOTLOADER_DEFINE_H

#include <stdint.h>

/**
Example flash oganization for nRF52832 and Mesh application **
//| Name                      | Value   |
//|:--------------------------|--------:|
//|`End Flash`                | 0x80000 |
//|`Bootloader settting`      | 0x7F000 |
//|`MBR parameter storage`    | 0x7E000 |
//|`OTA information`          | 0x7D000 |
//|`Bootloader`               | 0x6B000 |
//|'Mesh information'         |         |
//|`Application`              | 0x26000 |
//|`Sofdevice`                | 0x01000 |
//|`MBR`                      | 0x00000 |
*/

/*
**Example dlash oganization for nRRF5240**
| Name                      | Value   |
|:--------------------------|--------:|
|`End Flash`                |         |
|`Bootloader settting`      | 0xFF000 |
|`MBR parameter storage`    | 0xFE000 |

|`Bootloader end`           | 0xFDFFF | 
|`Bootloader`               | 0xE8000 |     // Size BOOTLOADER_SIZE=0x16000  

|`OTA information`          | 0xE7000 |     // User application will write data to this region to trigger ota update, @ref app_ota_info_t

|`Download firmware end`    | 0xE6FFF |      
|`Download firmware start`  | 0x87000 |     // OTA region 1

|`Application End`          | 0x86FFF |
|`Application Start`        | 0x27000 |     // OTA region 0

|`Sofdevice`                | 0x01000 |
|`MBR`                      | 0x00000 |
*/

#define APPLICATION_REGION_0_START_ADDDR    (0x27000)
#define APPLICATION_REGION_1_START_ADDDR    (0x87000)
#define APPLICATION_MAX_FILE_SIZE           (96*4096)
#define APPLICATION_BOOTLOADER_MAX_SIZE     (0x16000)
#define APPLICATION_OTA_INFORMATION         (0xE7000)

#define HTTP_MAX_URL_SIZE       (96)
#define HTTP_MAX_FILE_SIZE      (96)
#define HTTP_USER_NAME_MAX_SIZE (64)
#define HTTP_PASSWORD_MAX_SIZE  (64)
#define HTTP_DEBUG_MAX_SIZE     (32)

typedef enum
{
    APP_BOOTLOADER_NORMAL,      // Run normal application
    APP_BOOTLOADER_OTA = 0x12345678
} app_ota_flag_t;

typedef enum
{
    APP_OTA_REGION_0,
    APP_OTA_REGION_1,
    APP_OTA_REGION_INVALID = 0xFFFF     // 0xFFFFFFFF
} app_ota_region_t;

typedef struct __attribute((packed))
{
    uint8_t http_server[HTTP_MAX_URL_SIZE];
    uint8_t http_file[HTTP_MAX_FILE_SIZE];
    uint8_t http_user_name[HTTP_USER_NAME_MAX_SIZE];
    uint8_t http_password[HTTP_PASSWORD_MAX_SIZE];
    uint16_t http_port;
    uint8_t debug_info[HTTP_DEBUG_MAX_SIZE];
    app_ota_flag_t ota_flag;
    app_ota_region_t region;
} app_ota_info_t;

void app_bootloader_update_ota_address(void);

uint32_t app_bootloader_get_download_address(void);

uint32_t app_bootloader_get_application_boot_address(void);

void app_bootloader_ota_finish(char * debug_info);

#endif // APP_BOOTLOADER_DEFINE_H

