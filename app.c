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

static const char* MQTT_LIGHT_STATE_TOPIC              = "/light/status";
static const char* MQTT_LIGHT_COMMAND_TOPIC            = "/light/switch";
static const char* MQTT_LIGHT_BRIGHTNESS_STATE_TOPIC   = "/brightness/status";
static const char* MQTT_LIGHT_BRIGHTNESS_COMMAND_TOPIC = "/brightness/set";
static const char* MQTT_LIGHT_RGB_STATE_TOPIC          = "/rgb/status";
static const char* MQTT_LIGHT_RGB_COMMAND_TOPIC        = "/rgb/set";

static const char* MQTT_LIGHT_STATE_ON                 = "ON";
static const char* MQTT_LIGHT_STATE_OFF                = "OFF";

static const char* devicename = "home/livingroom/vitrine";

static os_timer_t wifi_timer;
MQTT_Client mqttClient;

static uint8_t rgb[3] = {0, 0, 0};
static uint8_t brightness = 255;
static uint8_t state = 0;

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
void led_init();
void led_on();
void led_off();

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

    MQTT_Connect(&mqttClient);
    WS2812_init();
    WS2812_set_length(4);
    led_off();
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
    char buffer[64];
    MQTT_Client* client = (MQTT_Client*) args;
    os_printf("MQTT: Connected\n");

    os_sprintf(buffer, "%s%s", devicename, MQTT_LIGHT_COMMAND_TOPIC);
	MQTT_Subscribe(client, buffer, 0);
    os_sprintf(buffer, "%s%s", devicename, MQTT_LIGHT_BRIGHTNESS_COMMAND_TOPIC);
	MQTT_Subscribe(client, buffer, 0);
    os_sprintf(buffer, "%s%s", devicename, MQTT_LIGHT_RGB_COMMAND_TOPIC);
	MQTT_Subscribe(client, buffer, 0);

    os_sprintf(buffer, "%s%s", devicename, MQTT_LIGHT_STATE_TOPIC);
    MQTT_Publish(client, buffer, MQTT_LIGHT_STATE_OFF, os_strlen(MQTT_LIGHT_STATE_OFF), 0, 0);
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
    char topicRet[64];
    char dataRet[12];

    MQTT_Client* client = (MQTT_Client*) args;
    os_memcpy(topicBuf, topic, topic_len);
    topicBuf[topic_len] = 0;
    os_memcpy(dataBuf, data, data_len);
    dataBuf[data_len] = 0;
    os_printf("Receive topic: %s, data: %s\n", topicBuf, dataBuf);

    char cmdtopic[64];
    os_sprintf(cmdtopic,"%s%s", devicename, MQTT_LIGHT_COMMAND_TOPIC);

    char brightnesstopic[64];
    os_sprintf(brightnesstopic,"%s%s", devicename, MQTT_LIGHT_BRIGHTNESS_COMMAND_TOPIC);

    char rgbtopic[64];
    os_sprintf(rgbtopic,"%s%s", devicename, MQTT_LIGHT_RGB_COMMAND_TOPIC);

    if (strcmp(topicBuf, cmdtopic) == 0) {
        if (strcmp(dataBuf, MQTT_LIGHT_STATE_ON) == 0) {
            os_sprintf(topicRet, "%s%s", devicename, MQTT_LIGHT_STATE_TOPIC);
            MQTT_Publish(client, topicRet, MQTT_LIGHT_STATE_ON, os_strlen(MQTT_LIGHT_STATE_ON), 0, 0);
            state = 1;
        } else { // if (strcmp(topicBuf, MQTT_LIGHT_STATE_OFF) == 0) {
            os_sprintf(topicRet, "%s%s", devicename, MQTT_LIGHT_STATE_TOPIC);
            MQTT_Publish(client, topicRet, MQTT_LIGHT_STATE_OFF, os_strlen(MQTT_LIGHT_STATE_OFF), 0, 0);
            state = 0;
        }
    } else if (strcmp(topicBuf, brightnesstopic) == 0) {
        uint8_t b = 0;
        uint8_t i = 0;

        while (i < data_len) {
            if (dataBuf[i] > 47 && dataBuf[i] < 58) {
                b = b * 10 + (dataBuf[i] - 48);
            }
            i++;
        }

        brightness = b;
        os_sprintf(topicRet, "%s%s", devicename, MQTT_LIGHT_BRIGHTNESS_STATE_TOPIC);
        os_sprintf(dataRet, "%u", brightness);
        MQTT_Publish(client, topicRet, dataRet, os_strlen(dataRet), 0, 0);

        os_printf("brightness: %s\n", dataRet);
    } else if (strcmp(topicBuf, rgbtopic) == 0) {
        uint8_t i = 0;
        uint8_t col[3] = { 0, 0, 0 };
        uint8_t color = 0;

        while (i < data_len) {
            if (dataBuf[i] > 47 && dataBuf[i] < 58) {
                col[color] = col[color] * 10 + (dataBuf[i] - 48);
            } else if (dataBuf[i] == ',') {
                color++;
            }
            i++;
        }

        if (1) {
            rgb[0] = col[0];
            rgb[1] = col[1];
            rgb[2] = col[2];

            os_sprintf(topicRet, "%s%s", devicename, MQTT_LIGHT_RGB_STATE_TOPIC);
            os_sprintf(dataRet, "%u,%u,%u", rgb[0], rgb[1], rgb[2]);
            MQTT_Publish(client, topicRet, dataRet, os_strlen(dataRet), 0, 0);
        }
    }

    WS2812_color_t color = {
            (uint8_t) rgb[0] * brightness / 255 * state,
            (uint8_t) rgb[1] * brightness / 255 * state,
            (uint8_t) rgb[2] * brightness / 255 * state
    };
    WS2812_update_leds(color);

    os_free(topicBuf);
    os_free(dataBuf);
}

void led_init()
{
    WS2812_set_led(0, WS2812_RED);
    WS2812_set_led(1, WS2812_GREEN);
    WS2812_set_led(2, WS2812_BLUE);
    WS2812_set_led(3, WS2812_WHITE);

    WS2812_update();
}

void led_on()
{
    WS2812_set_led(0, WS2812_ON);
    WS2812_set_led(1, WS2812_ON);
    WS2812_set_led(2, WS2812_ON);
    WS2812_set_led(3, WS2812_ON);

    WS2812_update();
}

void led_off()
{
    WS2812_set_led(0, WS2812_OFF);
    WS2812_set_led(1, WS2812_OFF);
    WS2812_set_led(2, WS2812_OFF);
    WS2812_set_led(3, WS2812_OFF);

    WS2812_update();
}
