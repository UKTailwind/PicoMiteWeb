#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// Generally you would define your own explicit list of lwIP options
// (see https://www.nongnu.org/lwip/2_1_x/group__lwip__opts.html)
//
// This example uses a common include to avoid repetition
#include "lwipopts_examples_common.h"
#undef TCP_WND
#define TCP_WND  16384

#define LWIP_ALTCP               1
#define LWIP_ALTCP_TLS           1
#define LWIP_ALTCP_TLS_MBEDTLS   1

#define LWIP_DEBUG 1
#define ALTCP_MBEDTLS_DEBUG  LWIP_DBG_ON

#endif
