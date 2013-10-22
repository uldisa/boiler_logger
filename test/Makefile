ARDUINO_DIR = /home/uldisa/Arduino/build/linux/work
ARDUINO_CORE_PATH = $(ARDUINO_DIR)/hardware/arduino/avr/cores/arduino
ARDUINO_VAR_PATH = $(ARDUINO_DIR)/hardware/arduino/avr/variants
export AVR_TOOLS_DIR=/home/uldisa/Arduino/build/linux/work/hardware/tools/avr
BOARD_TAG    = mega2560
MONITOR_PORT = /dev/ttyACM0
#ARDUINO_LIBS = SD
ARDUINO_VERSION=154
AVRDUDE=$(ARDUINO_DIR)/hardware/tools/avrdude
AVRDUDE_CONF=$(ARDUINO_DIR)/hardware/tools/avrdude.conf
LOCAL_SRCS=read.cpp
LOCAL_OBJ_FILES=read.o
TARGET=test
include ../arduino-mk/Arduino.mk

monitor:
	cu -l /dev/ttyACM0 -s 9600
