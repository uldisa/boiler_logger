include ../Makefile

LIBS=core MemoryFree
#PORT=/dev/ttyACM0
#PORT=COM11
#CPU=atmega2560
#BR=115200
#CPPFLAGS+=-DARDUINO_AVR_MEGA2560 -DARDUINO_ARCH_AVR
#VARIANT=mega
#PROGRAMMER=wiring
#PORT=/dev/ttyUSB0
PORT=COM9
CPU=atmega328p
BR=57600
CPPFLAGS+=-DARDUINO_AVR_PRO -DARDUINO_ARCH_AVR
VARIANT=standard
PROGRAMMER=arduino

#USER_LIB_PATH=$(CURDIR)/../libraries
include ../lib.mk
CPPFLAGS+=-Wall -Wextra -I. -Os -fno-exceptions -ffunction-sections -fdata-sections
accumulator.elf: accumulator.o PCD8544.o libMemoryFree.a  libcore.a