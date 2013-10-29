include ../Makefile

LIBS=core
#PORT=/dev/ttyACM1
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
include ../lib.mk
CPPFLAGS+=-I. -Os -fno-exceptions -ffunction-sections -fdata-sections
main.elf: main.o DS1302.o libcore.a
# compile project:
# make Blink.elf

# programm board:
# make Blink.prog

# monitor board:
# make monitur

# Cleanup
# make clean
AnalogReadSerial.elf:libcore.a

DS1302_Serial_Easy.elf:DS1302.o libcore.a
