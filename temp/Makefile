include ../Makefile

LIBS=core OneWire DallasTemperature
#PORT=/dev/ttyACM1
PORT=COM11
CPU=atmega2560
BR=115200
CPPFLAGS+=-DARDUINO_AVR_MEGA2560 -DARDUINO_ARCH_AVR
VARIANT=mega
PROGRAMMER=wiring
#PORT=COM9
#CPU=atmega328p
#BR=57600
#CPPFLAGS+=-DARDUINO_AVR_PRO -DARDUINO_ARCH_AVR
#VARIANT=standard
#PROGRAMMER=arduino
include ../lib.mk
#CPPFLAGS+=-I.
temp.elf: libOneWire.a libDallasTemperature.a libcore.a
m.elf: libOneWire.a libDallasTemperature.a libcore.a

p:temp.prog

# compile project:
# make Blink.elf

# programm board:
# make Blink.prog

# monitor board:
# make monitur

# Cleanup
# make clean
