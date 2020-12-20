#include <stdio.h>
#include <string.h>
#include "lwip/pbuf.h"
#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt_priv.h"
#include "lwip/netif.h"
#include "lwip/dns.h"
#include "gsm_context.h"
#include "gsm.h"
#include "mqtt_user.h"
#include "main.h"

#ifdef MQTT_WITH_SSL
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "lwip/altcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/priv/altcp_priv.h"
#endif

#define MQTT_KEEP_ALIVE_INTERVAL_SEC 600
#define APP_MQTT_HOST   "broker.hivemq.com"
#define APP_MQTT_PORT   1883
#define APP_MQTT_PERIOD_SEND_SUB_REQUEST_SEC    120

static char m_mqtt_sub_topic_name[48];
static ip_addr_t m_mqtt_server_address;
static mqtt_client_t m_mqtt_client;
static uint8_t m_dns_resolved = 0;
static uint8_t m_sub_req_error_count;


static mqtt_user_state_t m_mqtt_state = MQTT_USER_STATE_DISCONNECTED;

static void mqtt_sub_request_cb(void *arg, err_t result)
{
    /* Just print the result code here for simplicity, 
    normal behaviour would be to take some action if subscribe fails like 
    notifying user, retry subscribe or disconnect from server 
    */

    if (result != ERR_OK)
    {
        m_sub_req_error_count++;
        DebugPrint("Retrying [%d] send subscribe msg...\r\n", m_sub_req_error_count);
        if (m_sub_req_error_count >= 5)
        {
            /* close mqtt connection */
            mqtt_disconnect(&m_mqtt_client);

            m_sub_req_error_count = 0;
            m_mqtt_state = MQTT_USER_STATE_DISCONNECTED;
        }
        else
        {
            err_t err = mqtt_subscribe(&m_mqtt_client, 
                                        m_mqtt_sub_topic_name, 
                                        MQTT_USER_SUB_QoS, 
                                        mqtt_sub_request_cb, 
                                        arg);
        }
    }
    else
    {
        m_sub_req_error_count = 0;
        DebugPrint("Subscribed\r\n");
    }
}

/* 3. Implementing callbacks for incoming publish and data */
/* The idea is to demultiplex topic and create some reference to be used in data callbacks
   Example here uses a global variable, better would be to use a member in arg
   If RAM and CPU budget allows it, the easiest implementation might be to just take a copy of
   the topic string and use it in mqtt_incoming_data_cb
*/
static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
    DebugPrint("MQTT publish cb topic %s, length %u\r\n", topic, (unsigned int)tot_len);
}

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
    DebugPrint("MQTT data cb, length %d, flags %u\r\n", len, (unsigned int)flags);

    if (flags & MQTT_DATA_FLAG_LAST)
    {
        /* Last fragment of payload received (or whole part if payload fits receive buffer
            See MQTT_VAR_HEADER_BUFFER_LEN)  */
        //clear received buffer of client -> du lieu nhan lan sau khong bi thua cua lan truoc, 
        // neu lan truoc gui length > MQTT_VAR_HEADER_BUFFER_LEN
        memset(m_mqtt_client.rx_buffer, 0, MQTT_VAR_HEADER_BUFFER_LEN);
    }
    else
    {
            /* Handle fragmented payload, store in buffer, write to file or whatever */
    }
}

static void mqtt_client_connection_callback(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
   DebugPrint("mqtt_client_connection_callback reason: %d\r\n", status);

    err_t err;
    if (status == MQTT_CONNECT_ACCEPTED)
    {
        DebugPrint("mqtt_connection_cb: Successfully connected\r\n");
        m_mqtt_state = MQTT_USER_STATE_CONNECTED;

        /* Setup callback for incoming publish requests */
        mqtt_set_inpub_callback(client, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, arg);

        /* Subscribe to a topic named "fire/sub/IMEI" with QoS level 1, 
            call mqtt_sub_request_cb with result */
        DebugPrint("Subscribe %s\r\n", m_mqtt_sub_topic_name);
        err = mqtt_subscribe(client, m_mqtt_sub_topic_name, MQTT_USER_SUB_QoS, mqtt_sub_request_cb, arg);

        if (err != ERR_OK)
        {
           DebugPrint("mqtt_subscribe return: %d\r\n", err);
        }
    }
    else
    {
        /* Its more nice to be connected, so try to reconnect */
        m_mqtt_state = MQTT_USER_STATE_CONNECTING;
    }
}

/** -----------------------------------------------------------------
 * Using outgoing publish
 * Called when publish is complete either with sucess or failure
 */
static void mqtt_pub_request_cb(void *arg, err_t result)
{
    if (result != ERR_OK)
    {
        DebugPrint("Publish result: %d\r\n", result);
    }
    else
    {
        DebugPrint("Publish: OK\r\n");
    }
}

static void mqtt_client_send_heartbeat(void)
{
    DebugPrint("Ping....Implement later\r\n");
}

static err_t mqtt_client_send_subscribe_req(void)
{
    /* Subscribe to a topic named with QoS level 1, call mqtt_sub_request_cb with result */
    err_t err = mqtt_subscribe(&m_mqtt_client, m_mqtt_sub_topic_name, MQTT_USER_SUB_QoS, mqtt_sub_request_cb, NULL);

    DebugPrint("%s: topic %s\r\n", __FUNCTION__, m_mqtt_sub_topic_name);
}


int8_t mqtt_user_pub_msg(char *topic, char *content, uint16_t length)
{

	if (!mqtt_client_is_connected(&m_mqtt_client))
		return -1;


	/* Publish mqtt */
	err_t err = mqtt_publish(&m_mqtt_client, 
                            topic, 
                            content, 
                            length, 
                            MQTT_USER_PUB_QoS, 
                            MQTT_USER_RETAIN, 
                            mqtt_pub_request_cb, 
                            NULL);
	if (err != ERR_OK)
	{
		DebugPrint("Publish err: %d\r\n", err);
		return -1;
	}
    else
    {
        DebugPrint("Publish msg success\r\n");
    }
    
	return err;
}


void mqtt_set_sub_topic_name(char * name)
{
    snprintf(m_mqtt_sub_topic_name, sizeof(m_mqtt_sub_topic_name), "%s", name);
}

static int8_t mqtt_client_connect_to_broker(mqtt_client_t *client)
{
    static uint32_t idx = 0;
    char client_id[32];
    snprintf(client_id, sizeof(client_id), "%s_%d", "huytv", idx++ % 4096);
    struct mqtt_connect_client_info_t client_info = 
    {
        client_id,
        NULL, NULL,				  //User, pass
        MQTT_KEEP_ALIVE_INTERVAL_SEC, //Keep alive in seconds, 0 - disable
        NULL, NULL, 0, 0		  //Will topic, will msg, will QoS, will retain
    };

    /* Minimal amount of information required is client identifier, so set it here */
    //client_info.client_user = "";
    //client_info.client_pass = "";

    /* 
    * Initiate client and connect to server, if this fails immediately an error code is returned
    * otherwise mqtt_connection_cb will be called with connection result after attempting 
    * to establish a connection with the server. 
    * For now MQTT version 3.1.1 is always used 
    */
    err_t err = mqtt_client_connect(client, 
                                    &m_mqtt_server_address, 
                                    APP_MQTT_PORT, 
                                    mqtt_client_connection_callback, 
                                    0, 
                                    &client_info);

    /* For now just print the result code if something goes wrong */
    if (err != ERR_OK)
    {
        DebugPrint("mqtt_connect return %d\r\n", err);
        if (err == ERR_ISCONN)
        {
            DebugPrint("MQTT already connected\r\n");
        }
    }
    else
    {
        DebugPrint("Host %s-%s, client id %s\r\n", 
                    APP_MQTT_HOST, ipaddr_ntoa(&m_mqtt_server_address), 
                    client_id);
        DebugPrint("mqtt_client_connect: OK\r\n");
    }

    return err;
}

/**
 * @brief DNS found callback when using DNS names as server address.
 */
static void mqtt_dns_found_cb(const char *hostname, const ip_addr_t *ipaddr, void *arg)
{
    DebugPrint("mqtt_dns_found_cb: %s\r\n", hostname);

    LWIP_UNUSED_ARG(hostname);
    LWIP_UNUSED_ARG(arg);

    if (ipaddr != NULL)
    {
        /* Address resolved, send request */
        m_mqtt_server_address.addr = ipaddr->addr;
        DebugPrint("Server address resolved = %s\r\n", ipaddr_ntoa(&m_mqtt_server_address));
        m_dns_resolved = 1;
    }
    else
    {
        /* DNS resolving failed -> try another server */
        DebugPrint("mqtt_dns_found_cb: Failed to resolve server address resolved, trying next server\r\n");
        m_dns_resolved = 0;
    }
}

/**
 * @note Task should call every 1s
 */
void mqtt_user_polling_task(void)
{
    static uint8_t m_mqtt_tick_sec = 0;
    static uint32_t tick = 0, last_time_send_sub_req = 0;
      
    if (gsm_data_layer_is_ppp_connected())
    {
        m_mqtt_tick_sec++;
        switch (m_mqtt_state)
        {
            case MQTT_USER_STATE_DISCONNECTED:
                /* init client info...*/
                m_dns_resolved = 0;
                m_mqtt_state = MQTT_USER_STATE_RESOLVING_HOSTNAME;
                m_mqtt_tick_sec = 4;
                last_time_send_sub_req = 0;
                break;

            case MQTT_USER_STATE_RESOLVING_HOSTNAME:
                if (!m_dns_resolved)
                {
                    if (m_mqtt_tick_sec >= 5)
                    {
                        m_mqtt_tick_sec = 0;

                        DebugPrint("Getting host [%s] ip addr\r\n", APP_MQTT_HOST);
                        err_t err = dns_gethostbyname(
                                    APP_MQTT_HOST,
                                    &m_mqtt_server_address, 
                                    mqtt_dns_found_cb, 
                                    NULL);

                        if (err == ERR_INPROGRESS)
                        {
                            /* DNS request sent, wait for sntp_dns_found being called */
                            DebugPrint("dns request send: %d - Waiting for server address to be resolved\r\n", err);
                        }
                        else if (err == ERR_OK)
                        {
                            DebugPrint("dns resolved aready, host %s, mqtt_ipaddr = %s\r\n", 
                                        APP_MQTT_HOST,
                                        ipaddr_ntoa(&m_mqtt_server_address));
                            m_dns_resolved = 1;
                        }
                    }
                }
                else
                {
                    m_mqtt_tick_sec = 9;
                    m_mqtt_state = MQTT_USER_STATE_CONNECTING;
                }
                break;

            case MQTT_USER_STATE_CONNECTING:
                if (m_mqtt_tick_sec >= 10)
                {
                    if (mqtt_client_connect_to_broker(&m_mqtt_client) == ERR_OK)
                    {
                        m_mqtt_tick_sec = 5; /* Gui login sau 5s */
                    }
                    else
                    {
                        m_mqtt_tick_sec = 0;
                    }
                }
                break;

            case MQTT_USER_STATE_CONNECTED:
            {
                last_time_send_sub_req++;
                if (mqtt_client_is_connected(&m_mqtt_client))
                {
                    /* Send subscribe message periodic */
                    if ((last_time_send_sub_req % APP_MQTT_PERIOD_SEND_SUB_REQUEST_SEC) == 0)
                    {
                        if (mqtt_client_send_subscribe_req() != ERR_OK)
                        {
                            last_time_send_sub_req = APP_MQTT_PERIOD_SEND_SUB_REQUEST_SEC - 1;      // Resubcribe after 1s
                        }
                    }
                }
                else
                {
                    m_mqtt_state = MQTT_USER_STATE_DISCONNECTED;     
                }
            }
                break;
            default:
                    break;
        }
    }
    else 
    {
        m_mqtt_tick_sec = 4;
    }
}

