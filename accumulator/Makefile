include ../Makefile

LIBS=core MemoryFree DallasTemperature OneWire
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
accumulator.elf: accumulator.o PCD8544.o libMemoryFree.a libDallasTemperature.a libOneWire.a libcore.a
PCD8544.o test_LCD.o: PCD8544.h
TemperatureSensor.o test_LCD.o HumanInterface.o: TemperatureSensor.h
HumanInterface.o :HumanInterface.h

test_LCD.elf: test_LCD.o TemperatureSensor.o PCD8544.o BIGSERIF.o font5x8.o  HumanInterface.o libcore.a libDallasTemperature.a libOneWire.a 
main.elf: main.o TemperatureSensor.o PCD8544.o BIGSERIF.o font5x8.o  HumanInterface.o libcore.a libDallasTemperature.a libOneWire.a 
