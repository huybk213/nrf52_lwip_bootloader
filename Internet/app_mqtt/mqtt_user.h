#ifndef MQTT_USER_H
#define MQTT_USER_H

#include <stdint.h>

/* QoS value, 0 - at most once, 1 - at least once or 2 - exactly once */
#define MQTT_USER_SUB_QoS	1
#define MQTT_USER_PUB_QoS	1 		
#define MQTT_USER_RETAIN	0


typedef enum 
{
    MQTT_USER_STATE_DISCONNECTED = 0,
    MQTT_USER_STATE_RESOLVING_HOSTNAME,
    MQTT_USER_STATE_CONNECTING,
    MQTT_USER_STATE_CONNECTED
} mqtt_user_state_t;

/**
 * @brief Polling MQTT service
 * @note Task should call every second
 */
void mqtt_user_polling_task(void);

/**
 * @brief Publish message to mqtt broker
 * @param topic MQTT pub topic
 * @param content Message's content
 * @param length Message length
 * @return 0 success 
 *         Otherwise failed
 */
int8_t mqtt_user_pub_msg(char *topic, char *content, uint16_t length);

/**
 * @brief Set subscribte topic name
 * @param name : Topic name
 * @note : I only want to subscribte only 1 topic
 */
void mqtt_set_sub_topic_name(char * name);

#endif /* MQTT_USER_H */

