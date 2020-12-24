#ifndef HARDWARE_H
#define HARDWARE_H

#define NBIoT_BOARD
//#define SIM800C_BOARD

#ifdef SIM800C_BOARD

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

#endif /* SIM800C_BOARD */

#ifdef NBIoT_BOARD

#define HW_UART_TX_PIN                  NRF_GPIO_PIN_MAP(0,6)
#define HW_UART_RX_PIN                  NRF_GPIO_PIN_MAP(0,8)
#define HW_UART_RTS_PIN                 0xFFFFFFFF
#define HW_UART_CTS_PIN                 0xFFFFFFFF
#define HW_UART_FLOW_CONTROL            APP_UART_FLOW_CONTROL_DISABLED

#define HW_ADC_VBAT_PIN                 NRF_GPIO_PIN_MAP(0,4)


/* BG96 IO */
#define HW_BG96_RI_PIN                  NRF_GPIO_PIN_MAP(0,27)
#define HW_BG96_DTR_PIN                 NRF_GPIO_PIN_MAP(0,26)
#define HW_BG96_RTS_PIN                 NRF_GPIO_PIN_MAP(0,7)
#define HW_BG96_CTS_PIN                 NRF_GPIO_PIN_MAP(0,11)
#define HW_BG96_GPS_EN_PIN              NRF_GPIO_PIN_MAP(1,7)
#define HW_BG96_STATUS_PIN              NRF_GPIO_PIN_MAP(0,31)
#define HW_BG96_AP_READ_PIN             NRF_GPIO_PIN_MAP(0,30)
#define HW_BG96_W_DISABLE_PIN           NRF_GPIO_PIN_MAP(0,29)
#define HW_BG96_RESET_PIN               NRF_GPIO_PIN_MAP(0,28)
#define HW_BG96_POWER_KEY_PIN           NRF_GPIO_PIN_MAP(0,2)
#define HW_BG96_PSM_PIN                 NRF_GPIO_PIN_MAP(0,3)

/* nRF52 led */
#define HW_LED1_PIN                     NRF_GPIO_PIN_MAP(0,12)


#define HW_GPIO_PIN_INVALID 0xFFFFFFFF

#define HW_GSM_TX_PIN_NUMBER HW_UART_TX_PIN
#define HW_GSM_RX_PIN_NUMBER HW_UART_RX_PIN
#define HW_GSM_RTS_PIN_NUMBER HW_GPIO_PIN_INVALID // Mark as invalid
#define HW_GSM_CTS_PIN_NUMBER HW_GPIO_PIN_INVALID // Mark as invalid
#define HW_GSM_UART_HWFC APP_UART_FLOW_CONTROL_DISABLED

#define HW_GSM_POWER_KEY_PIN HW_BG96_POWER_KEY_PIN
#define HW_GSM_STATUS_PIN HW_BG96_STATUS_PIN
#define HW_GSM_RESET_PIN HW_BG96_RESET_PIN

#endif /* NBIoT_BOARD */

#endif /* HARDWARE_H */