PROGRAM       = esp-led-controller
FLASH_SIZE    = 4m
FLASH_MODE    = qio
ESPBAUD       = 921600
ESPPORT       = /dev/ttyUSB0

EXTRA_SRCS     = $(ROOT)/sdk/driver_lib

include $(ESP_EASY_SDK)/common.mk
