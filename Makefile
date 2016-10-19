PROGRAM       = esp-led-controller
FLASH_SIZE    = 32m
FLASH_MODE    = qio
ESPBAUD       = 921600
ESPPORT       = COM3

EXTRA_SRCS     = $(ROOT)/sdk/driver_lib

include $(ESP_EASY_SDK)/common.mk
