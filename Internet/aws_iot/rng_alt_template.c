
/**
  *  Portions COPYRIGHT 2018 STMicroelectronics, All Rights Reserved
  *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
  *
  ******************************************************************************
  * @file    rng_alt_template.c
  * @author  MCD Application Team
  * @brief   mbedtls alternate entropy data function.
  *          the mbedtls_hardware_poll() is customized to use the STM32 RNG
  *          to generate random data, required for TLS encryption algorithms.
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Apache 2.0 license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  * https://opensource.org/licenses/Apache-2.0
  *
  ******************************************************************************
  */

#include "mbedtls/entropy_poll.h"
#include "mbedtls/platform.h"
#include "stdint.h"
#include MBEDTLS_CONFIG_FILE
#include "mbedtls/entropy_poll.h"
#include "mbedtls/platform.h"
#include "stdint.h"
//#include "rand.h"
/*
 * include the correct headerfile depending on the nRF52 family 
*/
#include <nrf_soc.h>
#include <string.h>

uint32_t rand_hw_rng_get(uint8_t* p_result, uint16_t len)
{
#ifdef SOFTDEVICE_PRESENT
    uint32_t error_code;
    uint8_t bytes_available;
    uint32_t count = 0;
    while (count < len)
    {
        do
        {
            sd_rand_application_bytes_available_get(&bytes_available);
        } while (bytes_available == 0);

        if (bytes_available > len - count)
        {
            bytes_available = len - count;
        }

        error_code =
            sd_rand_application_vector_get(&p_result[count],
            bytes_available);
        if (error_code != NRF_SUCCESS)
        {
            return NRF_ERROR_INTERNAL;
        }

        count += bytes_available;
    }
#else
    NRF_RNG->TASKS_START = 1;
    while (len)
    {
        while (!NRF_RNG->EVENTS_VALRDY);
        p_result[--len] = NRF_RNG->VALUE;
        NRF_RNG->EVENTS_VALRDY = 0;
    }
    NRF_RNG->TASKS_STOP = 1;
#endif
    return NRF_SUCCESS;
}


int mbedtls_hardware_poll( void *Data, unsigned char *Output, size_t Len, size_t *oLen )
{
  uint32_t index;
  uint32_t random_value;
//  int ret;

  rand_hw_rng_get(Output, Len);
  *oLen = Len;
//  for (index = 0; index < Len/4; index++)
//  {
//    if (rand() == HAL_OK)
//    {
//        *oLen += 4;
//        memset(&(Output[index * 4]), (int)random_value, 4);
//    }
//  }
//  ret = 0;

  return 0;
}

char ran_buf[64];
//char *temp = “somerandom stringi havegivenasthesource”;
int mbedtls_platform_std_nv_seed_read( unsigned char *buf, size_t buf_len )
{
    rand_hw_rng_get(buf, buf_len);
    return( buf_len );
}

int mbedtls_platform_std_nv_seed_write( unsigned char *buf, size_t buf_len )
{
    memcpy(ran_buf, buf, buf_len > 64 ? 64 : buf_len);
    return( buf_len );
}