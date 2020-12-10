#ifndef __HARDWARE_H__
#define __HARDWARE_H__

#define HW_GPIO_LED_GSM_B_PIN 28 // GSM Blue led
#define HW_GPIO_LED_GSM_R_PIN 29 // GSM Red led

#define HW_GPIO_LED_PWR_R_PIN 22 // Led power red
#define HW_GPIO_LED_PWR_B_PIN 23 // Led power blue

#define HW_GPIO_LED_STATUS_B_PIN 12
#define HW_GPIO_LED_STATUS_R_PIN 13

#define HW_GPIO_PIN_INVALID 0xFFFFFFFF

#define HW_GSM_TX_PIN_NUMBER 9
#define HW_GSM_RX_PIN_NUMBER 10
#define HW_GSM_RTS_PIN_NUMBER HW_GPIO_PIN_INVALID // Mark as invalid
#define HW_GSM_CTS_PIN_NUMBER HW_GPIO_PIN_INVALID // Mark as invalid
#define HW_GSM_UART_HWFC APP_UART_FLOW_CONTROL_DISABLED

#define HW_GSM_POWER_KEY_PIN 4
#define HW_GSM_STATUS_PIN 5

#define HW_BTN_ARARM 16
#define HW_BTN_OPTIONAL 26

#define HW_BTN_HW_CONFIG                           \
    {                                              \
        /* PinName       Last state   Idle level*/ \
        {HW_BTN_ARARM, 1, 1},                      \
            {HW_BTN_OPTIONAL, 1, 1},               \
    }

#endif // __HARDWARE_H__
