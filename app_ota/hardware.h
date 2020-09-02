#ifndef __HARDWARE_H__
#define __HARDWARE_H__

#define HW_GPIO_LED_GSM_B_PIN			    6       // GSM Blue led
#define HW_GPIO_LED_GSM_R_PIN			    7       // GSM Red led

#define HW_GPIO_LED_PWR_R_PIN		        8
#define HW_GPIO_LED_PWR_B_PIN			    9

#define HW_GPIO_LED_STATUS_B_PIN			10
#define HW_GPIO_LED_STATUS_R_PIN			11


#define HW_GSM_TX_PIN_NUMBER	            16
#define HW_GSM_RX_PIN_NUMBER	            17
#define HW_GSM_RTS_PIN_NUMBER              0xFFFFFFFF      // Mark as invalid
#define HW_GSM_CTS_PIN_NUMBER              0xFFFFFFFF      // Mark as invalid
#define HW_GSM_UART_HWFC                   APP_UART_FLOW_CONTROL_DISABLED

#define HW_GSM_POWER_KEY_PIN		    13
#define HW_GSM_PWR_CTRL_PIN		        20
#define GSM_MUTE_PIN_NUMBER	            19				//Active LOW

#define HW_GSM_STATUS_PIN		        14
#define HW_GSM_RI_PIN                   15
#define HW_GSM_NET_LIGHT_PIN		    18


// #define LED_IND_PIN                         HW_GPIO_LED_STATUS_B_PIN
#define HW_BTN_HW_CONFIG                        \
    {                                              \
        /* PinName       Last state   Idle level*/ \
        {GPIO_CENTER_BUTTON_PIN, 1, 1},                                \
        {GPIO_USER_BUTTON_PIN, 1, 1},                                \
    }


#endif // __HARDWARE_H__

