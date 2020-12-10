/******************************************************************************
 * @file    	GSM_Utilities.c
 * @author  	
 * @version 	V1.0.0
 * @date    	15/01/2014
 * @brief   	
 ******************************************************************************/

/******************************************************************************
                                   INCLUDES					    			 
 ******************************************************************************/
#include <stdlib.h>
#include <string.h>
#include "gsm.h"
#include "gsm_context.h"
#include "gsm_utilities.h"

void gsm_utilities_get_imei(uint8_t *imei_buffer, uint8_t *result)
{
    uint8_t count = 0;
    uint8_t tmp_count = 0;

    for (count = 0; count < strlen((char *)imei_buffer); count++)
    {
        if (imei_buffer[count] >= '0' && imei_buffer[count] <= '9')
        {
            result[tmp_count++] = imei_buffer[count];
        }

        if (tmp_count >= GSM_IMEI_MAX_LENGTH)
        {
            break;
        }
    }

    result[tmp_count] = 0;
}

bool gsm_utilities_get_signal_strength_from_buffer(char *buffer, uint8_t *csq)
{
    // assert(buffer);

    char *temp = strstr((char *)buffer, "+CSQ:");

    if (temp == NULL)
        return false;

    *csq = gsm_utilities_get_number_from_string(6, temp);

    return true;
}

uint32_t gsm_utilities_get_number_from_string(uint16_t index, char *buffer)
{
    // assert(buffer);

    uint32_t value = 0;
    uint16_t tmp = index;

    while (buffer[tmp] && tmp < 1024)
    {
        if (buffer[tmp] >= '0' && buffer[tmp] <= '9')
        {
            value *= 10;
            value += buffer[tmp] - 48;
        }
        else
        {
            break;
        }
        tmp++;
    }

    return value;
}
