include ../Makefile

LIBS=core
#PORT=/dev/ttyACM1
#PORT=COM11
#CPU=atmega2560
#BR=115200
#CPPFLAGS+=-DARDUINO_AVR_MEGA2560 -DARDUINO_ARCH_AVR
#VARIANT=mega
#PROGRAMMER=wiring
PORT=COM9
CPU=atmega328p
BR=57600
CPPFLAGS+=-DARDUINO_AVR_PRO -DARDUINO_ARCH_AVR
VARIANT=standard
PROGRAMMER=arduino
include ../lib.mk
CPPFLAGS+=-Wall -Wextra -I. -Os -fno-exceptions -ffunction-sections -fdata-sections
ind.elf: libcore.a

