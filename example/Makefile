include ../Makefile

LIBS=core
#PORT=/dev/ttyACM1
#CPU=atmega2560
#CPPFLAGS+=-DARDUINO_AVR_MEGA2560 -DARDUINO_ARCH_AVR
#VARIANT=mega
PORT=COM9
CPU=atmega328p
CPPFLAGS+=-DARDUINO_AVR_MINI -DARDUINO_ARCH_AVR
VARIANT=eightanaloginputs
include ../lib.mk

Blink.elf: libcore.a

# compile project:
# make Blink.elf

# programm board:
# make Blink.prog

# monitor board:
# make monitur

# Cleanup
# make clean
