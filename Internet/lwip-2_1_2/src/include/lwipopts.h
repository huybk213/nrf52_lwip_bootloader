/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Simon Goldschmidt
 *
 */
#ifndef LWIP_HDR_LWIPOPTS_H
#define LWIP_HDR_LWIPOPTS_H

// ======================= FOR SYSTEM ====================//
/* We link to special sys_arch.c (for basic non-waiting API layers unit tests) */
#define NO_SYS                          0
#define SYS_LIGHTWEIGHT_PROT            0
#define LWIP_NETCONN                    !NO_SYS
#define LWIP_SOCKET                     !NO_SYS
#define LWIP_STATS                      0

#define LWIP_SKIP_PACKING_CHECK

// ================= FOR NETWORK INTERFACE ==============//
#define PPP_SUPPORT                     1
#define PPPOS_SUPPORT	1
//#define PAP_SUPPORT	1
#define CHAP_SUPPORT	0
//#define PPP_AUTH_SUPPORT (PAP_SUPPORT || CHAP_SUPPORT || EAP_SUPPORT)

// ======================= FOR DEBUG =====================//
//#define LWIP_TESTMODE                   1

#define LWIP_DEBUG	LWIP_DBG_ON		//ON : Tang dung luong hex len khoang 5KB!
//#define PPP_DEBUG 1
//#define DNS_DEBUG	LWIP_DBG_ON
//#define LWIP_PLATFORM_DIAG(x) printf("\r\n"); printf x


// ====================== FOR TCP/IP FUNCTION ===============//
#define LWIP_IPV4            1
//#define LWIP_IPV6                       1

#define LWIP_ARP	0
//#define LWIP_RAW	1
#define LWIP_TCP 1
#define LWIP_UDP 1
#define LWIP_ICMP 1
#define LWIP_DNS 1

#define LWIP_RAND() ((u32_t)rand())

#define TCP_TTL	255
//#define LWIP_TCP_KEEPALIVE 1

#define SNTP_UPDATE_DELAY           60000	/* ms */

// ===================== TCP_TLS - ENCRYPTED =================//
#ifdef MQTT_WITH_SSL
#define LWIP_ALTCP 	1
#define LWIP_ALTCP_TLS 1
#define LWIP_ALTCP_TLS_MBEDTLS 1
#define ALTCP_MBEDTLS_DEBUG LWIP_DBG_ON
#define PBUF_POOL_SIZE 48
#define TCP_WND (32 * TCP_MSS)
#define ALTCP_MBEDTLS_MEM_DEBUG LWIP_DBG_ON
#endif /* MQTT_WITH_SSL */

// ====================== MQTT & HTTP =======================//
#define LWIP_CALLBACK_API		1		/* MQTT & HTTP */
#define LWIP_HTTPC_HAVE_FILE_IO   0
#define HTTP_DEFAULT_PORT   LWIP_IANA_PORT_HTTP

/**
 * Number of bytes in receive buffer, must be at least the size of the longest incoming topic + 8
 * If one wants to avoid fragmented incoming publish, set length to max incoming topic length + max payload length + 8
 */
 #define MQTT_VAR_HEADER_BUFFER_LEN 512		//default: 128
 
// ====================== FOR STATUS CALLBACK ==============//
#define LWIP_NETIF_STATUS_CALLBACK 1
#define LWIP_NETIF_LINK_CALLBACK	1
#define PPP_NOTIFY_PHASE 1
//#define LINK_STATS 1

#define DNS_TABLE_SIZE        (3) 
#define DNS_MAX_NAME_LENGTH   (128)

#ifndef LWIP_PLATFORM_ASSERT
#define LWIP_PLATFORM_ASSERT
#endif 

// ====================== FOR CHECKSUM =====================//
//#define LWIP_CHECKSUM_ON_COPY           1
//#define TCP_CHECKSUM_ON_COPY_SANITY_CHECK 1
//#define TCP_CHECKSUM_ON_COPY_SANITY_CHECK_FAIL(printfmsg) LWIP_ASSERT("TCP_CHECKSUM_ON_COPY_SANITY_CHECK_FAIL", 0)

//===================== REDUCE CODE SIZE ===================//
//using the C runtime memory functions (malloc, free, etc.) => to reduce code size
#define MEM_LIBC_MALLOC 1		//Giam duoc 1KB code size

//to prevent assertion code being included => reduce code
//#define LWIP_NOASSERT  1		//Giam duoc vai chuc KB code size!


//#define LWIP_NETCONN_FULLDUPLEX         LWIP_SOCKET
//#define LWIP_NETBUF_RECVINFO            1
//#define LWIP_HAVE_LOOPIF                1
//#define TCPIP_THREAD_TEST

/* Enable DHCP to test it, disable UDP checksum to easier inject packets */
//#define LWIP_DHCP                       1


//Gia tri mac dinh cua lwIP
/**
 * TCP_MSS: TCP Maximum segment size. (default is 536, a conservative default,
 * you might want to increase this.)
 * For the receive side, this MSS is advertised to the remote side
 * when opening a connection. For the transmit size, this MSS sets
 * an upper limit on the MSS advertised by the remote host.
 */
#define TCP_MSS                         1460

/**
 * TCP_SND_BUF: TCP sender buffer space (bytes).
 * To achieve good performance, this should be at least 2 * TCP_MSS.
 */
#define TCP_SND_BUF                     (3 * TCP_MSS)

/**
 * TCP_SND_QUEUELEN: TCP sender buffer space (pbufs). This must be at least
 * as much as (2 * TCP_SND_BUF/TCP_MSS) for things to work.
 */
//#define TCP_SND_QUEUELEN                ((4 * (TCP_SND_BUF) + (TCP_MSS - 1))/(TCP_MSS))

/**
 * MEMP_NUM_TCP_SEG: the number of simultaneously queued TCP segments.
 * (requires the LWIP_TCP option)
 */
//#define MEMP_NUM_TCP_SEG                16

/**
 * MEMP_NUM_TCP_PCB: the number of simultaneously active TCP connections.
 * (requires the LWIP_TCP option)
 */
//#define MEMP_NUM_TCP_PCB                8		//5 - default

/**
 * MEMP_NUM_TCP_PCB_LISTEN: the number of listening TCP connections.
 * (requires the LWIP_TCP option)
 */
//#define MEMP_NUM_TCP_PCB_LISTEN         8		//8 - default

/**
 * MEM_SIZE: the size of the heap memory. If the application will send
 * a lot of data that needs to be copied, this should be set high.
 */
#define MEM_SIZE                        96000		//1600 - default
//#define LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT 	1

/**
 * PBUF_POOL_SIZE: the number of buffers in the pbuf pool.
 */
//#define PBUF_POOL_SIZE          32  	//16 - default


/* Minimal changes to opt.h required for tcp unit tests: */
//#define MEM_SIZE                        (10*1024)
//#define TCP_MSS                         536
//#define TCP_SND_BUF                     (4 * TCP_MSS)	//12
//#define TCP_WND                         (3 * TCP_MSS)	//10
//#define TCP_SND_QUEUELEN                (4 * TCP_SND_BUF / TCP_MSS)		//40
//#define MEMP_NUM_TCP_SEG                TCP_SND_QUEUELEN + 1	//TCP_SND_QUEUELEN


//#define MEM_SIZE                        1600		//MEM_SIZE: the size of the heap memory, default: 1600
//#define TCP_SND_QUEUELEN                7	//40			//Dam bao TCP_SND_QUEUELEN > (2 * (TCP_SND_BUF / TCP_MSS))
//#define MEMP_NUM_TCP_SEG                TCP_SND_QUEUELEN
//#define TCP_SND_BUF                     (3 * TCP_MSS)		//TCP_MSS: TCP Maximum segment size. (default is 536)
//#define TCP_WND                         (3 * TCP_MSS)

///* MEM_SIZE: the size of the heap memory. If the application will send
//a lot of data that needs to be copied, this should be set high. */
//#define MEM_SIZE                (10 *1024)

///* MEMP_NUM_PBUF: the number of memp struct pbufs. If the application
//   sends a lot of data out of ROM (or other static memory), this
//   should be set high. */
//#define MEMP_NUM_PBUF           50
///* MEMP_NUM_UDP_PCB: the number of UDP protocol control blocks. One
//   per active UDP "connection". */
//#define MEMP_NUM_UDP_PCB        6
///* MEMP_NUM_TCP_PCB: the number of simulatenously active TCP
//   connections. */
//#define MEMP_NUM_TCP_PCB        10
///* MEMP_NUM_TCP_PCB_LISTEN: the number of listening TCP
//   connections. */
//#define MEMP_NUM_TCP_PCB_LISTEN 5
///* MEMP_NUM_TCP_SEG: the number of simultaneously queued TCP
//   segments. */
//#define MEMP_NUM_TCP_SEG        12
///* MEMP_NUM_SYS_TIMEOUT: the number of simulateously active
//   timeouts. */
//#define MEMP_NUM_SYS_TIMEOUT    10

/*
   ------------------------------------
   ---------- Socket options ----------
   ------------------------------------
*/
/**
 * LWIP_SOCKET==1: Enable Socket API (require to use sockets.c)
 */
//#define LWIP_SOCKET                     1	//ONLY USE WITH NO_SYS = 0

/**
 * LWIP_NETCONN==1: Enable Netconn API (require to use api_lib.c)
 */
//#define LWIP_NETCONN                    0


//#define LWIP_WND_SCALE                  1
//#define TCP_RCV_SCALE                   0
//#define PBUF_POOL_SIZE                  400 /* pbuf tests need ~200KByte */

/* Enable IGMP and MDNS for MDNS tests */
//#define LWIP_IGMP                       1
//#define LWIP_MDNS_RESPONDER             1
//#define LWIP_NUM_NETIF_CLIENT_DATA      (LWIP_MDNS_RESPONDER)

/* Minimal changes to opt.h required for etharp unit tests: */
#define ETHARP_SUPPORT_STATIC_ENTRIES   1

//#define MEMP_NUM_SYS_TIMEOUT            (LWIP_NUM_SYS_TIMEOUT_INTERNAL + 8)

/* MIB2 stats are required to check IPv4 reassembly results */
//#define MIB2_STATS                      1

/* netif tests want to test this, so enable: */
//#define LWIP_NETIF_EXT_STATUS_CALLBACK  1

/* Check lwip_stats.mem.illegal instead of asserting */
//#define LWIP_MEM_ILLEGAL_FREE(msg)      /* to nothing */

#define LWIP_PROVIDE_ERRNO

#endif /* LWIP_HDR_LWIPOPTS_H */
