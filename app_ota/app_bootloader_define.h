

#ifndef APP_BOOTLOADER_DEFINE_H
#define APP_BOOTLOADER_DEFINE_H

#include <stdint.h>

/*
**Flash oganization for nRF52832 **
| Name                      | Value   |
|:--------------------------|--------:|
|`End Flash`                | 0x80000 |
|`Bootloader settting`      | 0x7F000 |
|`MBR parameter storage`    | 0x7E000 |
|`OTA information`          | 0x7D000 |
|`Bootloader`               | 0x6B000 |
|'Mesh information'         |         |
|`Application`              | 0x26000 |
|`Sofdevice`                | 0x01000 |
|`MBR`                      | 0x00000 |
*/

#define APPLICATION_START_ADDDR             (0x26000)
#define APPLICATION_MAX_FILE_SIZE           (54*4096)
#define APPLICATION_BOOTLOADER_MAX_SIZE     (0x12000)
#define APPLICATION_OTA_INFORMATION         (0x7D000)

#define HTTP_MAX_URL_SIZE       (96)
#define HTTP_MAX_FILE_SIZE      (96)
#define HTTP_USER_NAME_MAX_SIZE (64)
#define HTTP_PASSWORD_MAX_SIZE  (64)
#define HTTP_DEBUG_MAX_SIZE     (32)

typedef enum
{
    APP_BOOTLOADER_NORMAL,
    APP_BOOTLOADER_OTA = 0x12345678
} app_ota_flag_t;

typedef struct __attribute((packed))
{
    uint8_t http_server[HTTP_MAX_URL_SIZE];
    uint8_t http_file[HTTP_MAX_FILE_SIZE];
    uint8_t http_user_name[HTTP_USER_NAME_MAX_SIZE];
    uint8_t http_password[HTTP_PASSWORD_MAX_SIZE];
    uint16_t http_port;
    uint8_t debug_info[HTTP_DEBUG_MAX_SIZE];
    app_ota_flag_t ota_flag;
} app_ota_info_t;

void app_bootloader_set_ota_done(char * debug_info);

#endif // APP_BOOTLOADER_DEFINE_H

