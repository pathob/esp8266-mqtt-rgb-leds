#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "ets_sys.h"
#include "gpio.h"
#include "osapi.h"
#include "mem.h"

#include "driver/uart.h"
#include "lib/httplib.h"
#include "lib/ws2812.h"
#include "lib/mqtt/mqtt.h"

#include "user_config.h"
#include "wifi_config.h"

static const uint8 led_pin = 2;
static uint8 led_active = 0;

static os_timer_t led_timer;
static os_timer_t wifi_timer;

// listening connection data
static struct espconn httpdConn;
static esp_tcp httpdTcp;
MQTT_Client mqttClient;

void user_init();
void sys_init_done_cb();
void MQTT_init();
static void MQTT_connected_cb(
        uint32_t *args);
static void MQTT_disconnected_cb(
        uint32_t *args);
static void MQTT_published_cb(
        uint32_t *args);
static void MQTT_data_cb(
        uint32_t *args,
        const char* topic,
        uint32_t topic_len,
        const char *data,
        uint32_t data_len);
void WIFI_connect_cb(
        uint8_t status);
void httpd_init();
void httpd_conn_cb(
        void *arg);
void httpd_recv_cb(
        void *arg,
        char *data,
        unsigned short len);
void blinky();

void user_init()
{
    UART_SetBaudrate(UART0, BIT_RATE_115200);
    os_printf("\nSDK version: %s", system_get_sdk_version());
    wifi_set_opmode(STATION_MODE);
    system_init_done_cb(sys_init_done_cb);
}

void sys_init_done_cb()
{
    MQTT_init();
    WIFI_connect(WIFI_connect_cb);
    // httpd_init();

    MQTT_Connect(&mqttClient);
    WS2812_init();

    blinky();
    // blinky();
    // blinky();
}

void WIFI_connect_cb(
        uint8_t status)
 {
    if (status == STATION_GOT_IP) {
        os_printf("MQTT_Connect\n");
        MQTT_Connect(&mqttClient);
    } else {
        os_printf("MQTT_Disconnect\n");
        MQTT_Disconnect(&mqttClient);
    }
}

void MQTT_init()
{
    MQTT_InitConnection(&mqttClient, MQTT_HOST, MQTT_PORT, DEFAULT_SECURITY);

    MQTT_InitClient(&mqttClient, MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS,
            MQTT_KEEPALIVE, MQTT_CLEAN_SESSION);
    MQTT_InitLWT(&mqttClient, "/lwt", "offline", 0, 0);
    MQTT_OnConnected(&mqttClient, MQTT_connected_cb);
    MQTT_OnDisconnected(&mqttClient, MQTT_disconnected_cb);
    MQTT_OnPublished(&mqttClient, MQTT_published_cb);
    MQTT_OnData(&mqttClient, MQTT_data_cb);
}

static void MQTT_connected_cb(
        uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*) args;
    os_printf("MQTT: Connected\n");

    MQTT_Subscribe(client, "led", 0);

    // MQTT_Subscribe(client, "mqtt/topic/1", 1);
    // MQTT_Subscribe(client, "mqtt/topic/2", 2);

    // MQTT_Publish(client, "mqtt/topic/0", "hello0", 6, 0, 0);
    // MQTT_Publish(client, "mqtt/topic/1", "hello1", 6, 1, 0);
    // MQTT_Publish(client, "mqtt/topic/2", "hello2", 6, 2, 0);
}

static void MQTT_disconnected_cb(
        uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*) args;
    os_printf("MQTT: Disconnected\n");
}

static void MQTT_published_cb(
        uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*) args;
    os_printf("MQTT: Published\n");
}

static void MQTT_data_cb(
        uint32_t *args,
        const char* topic,
        uint32_t topic_len,
        const char *data,
        uint32_t data_len)
{
    char *topicBuf = (char*) os_zalloc(topic_len + 1);
    char *dataBuf = (char*) os_zalloc(data_len + 1);

    MQTT_Client* client = (MQTT_Client*) args;
    os_memcpy(topicBuf, topic, topic_len);
    topicBuf[topic_len] = 0;
    os_memcpy(dataBuf, data, data_len);
    dataBuf[data_len] = 0;
    os_printf("Receive topic: %s, data: %s\n", topicBuf, dataBuf);

    if (strcmp(topicBuf, "led") == 0) {

        uint8_t i = 0;
        uint8_t color = 0;
        uint8_t grb[3] = {0, 0, 0};
        uint8_t leds[12];

        while (i < data_len) {
            if (dataBuf[i] > 47 && dataBuf[i] < 58) {
                grb[color] = grb[color] * 10 + (dataBuf[i] - 48);
            } else if (dataBuf[i] == ';') {
                color++;
            }
            i++;
        }

        os_memcpy(&leds[0], grb, sizeof(grb));
        os_memcpy(&leds[3], grb, sizeof(grb));
        os_memcpy(&leds[6], grb, sizeof(grb));
        os_memcpy(&leds[9], grb, sizeof(grb));

        WS2812_write(leds, sizeof(leds));
    }

    os_free(topicBuf);
    os_free(dataBuf);
}

//
void httpd_init()
{
    httpdConn.type = ESPCONN_TCP;
    httpdConn.state = ESPCONN_NONE;
    httpdTcp.local_port = 80;
    httpdConn.proto.tcp = &httpdTcp;

    os_printf("Httpd init, conn=%p\n", &httpdConn);
    espconn_regist_connectcb(&httpdConn, httpd_conn_cb);
    espconn_accept(&httpdConn);
}

//Callback called when there's data available on a socket.
void httpd_recv_cb(
        void *arg,
        char *data,
        unsigned short len)
{
    struct espconn *conn = arg;

    char method[5];
    char location[256];

    http_request_method(data, method, 5);
    http_request_location(data, location, 256);

    if (strcmp(method,"GET") == 0) {

        if (strcmp(location,"/") == 0) {
            if (led_active == 0) {
                // gpio_output_set(0, (1 << led_pin), 0, 0);
                led_active = 1;
                espconn_sent(conn, "LED is now ON", 13);
            } else {
                // gpio_output_set((1 << led_pin), 0, 0, 0);
                led_active = 0;
                espconn_sent(conn, "LED is now OFF", 14);
            }
        }
    }

    espconn_disconnect(conn);
}

void httpd_conn_cb(
        void *arg)
{
    struct espconn *conn = arg;
    // only set recv callback, skip others
    espconn_regist_recvcb(conn, httpd_recv_cb);
    espconn_set_opt(conn, ESPCONN_NODELAY);
}

void blinky()
{
    uint8 array[] = {0, 0, 255};
    WS2812_write(array, 3);
}
