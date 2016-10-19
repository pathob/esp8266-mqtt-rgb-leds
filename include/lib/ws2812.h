#ifndef _WS2812_H
#define _WS2812_H

#include "ets_sys.h"
#include "osapi.h"
#include "c_types.h"
#include "user_interface.h"
#include "gpio.h"

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

void WS2812_init();

void WS2812_write(
        const uint8 *buffer,
        uint8 length);

#endif
