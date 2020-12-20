#ifndef APP_DEBUG_H
#define APP_DEBUG_H

#include <stdint.h>

#include "nrf_log.h"


//#define DebugPrint      NRF_LOG_INFO
#ifndef DebugPrint
#define DebugPrint      printf
#endif

#endif // !APP_DEBUG_H



