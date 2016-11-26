#ifndef _WS2812_H
#define _WS2812_H

#include "ets_sys.h"
#include "osapi.h"
#include "c_types.h"
#include "user_interface.h"
#include "gpio.h"
#include "mem.h"

#include "driver/uart_register.h"

#define WSGPIO 2

#define GPIO_LOW        0
#define GPIO_HIGH       1

#define LED_PIN         2
#define LED_PIN_MUX     PERIPHS_IO_MUX_GPIO2_U
#define LED_PIN_FUNC    FUNC_GPIO2

#define LED_PIN_HIGH    GPIO_OUTPUT_SET(LED_PIN, GPIO_HIGH)
#define LED_PIN_LOW     GPIO_OUTPUT_SET(LED_PIN, GPIO_LOW)

#define BLA             1<<2

//You will have to     os_intr_lock();      os_intr_unlock();

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} WS2812_color_t;

#define WS2812_OFF       (WS2812_color_t) {   0,   0,   0 }
#define WS2812_ON        (WS2812_color_t) { 255, 255, 255 }

#define WS2812_RED       (WS2812_color_t) {  10,   0,   0 }
#define WS2812_GREEN     (WS2812_color_t) {   0,  10,   0 }
#define WS2812_BLUE      (WS2812_color_t) {   0,   0,  10 }
#define WS2812_WHITE     (WS2812_color_t) {  10,  10,  10 }
#define WS2812_SUNSET    (ES2812_color_t) {  45; 121;  13 }


void WS2812_init();

void WS2812_set_length(
        uint16_t len);

void WS2812_set_led(
        uint16_t pos,
        WS2812_color_t color);

void WS2812_update();

void WS2812_update_leds(
        WS2812_color_t color);

void WS2812_write(
        const uint8 *buffer,
        uint16 length);

#endif
