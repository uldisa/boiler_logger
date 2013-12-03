include ../Makefile

LIBS=core MemoryFree OneWire DallasTemperature SD SPI UIPEthernet
PORT=/dev/ttyACM0
#PORT=COM11
CPU=atmega2560
BR=115200
CPPFLAGS+=-DARDUINO_AVR_MEGA2560 -DARDUINO_ARCH_AVR
VARIANT=mega
PROGRAMMER=wiring
#PORT=/dev/ttyUSB0
#PORT=COM9
#CPU=atmega328p
#BR=57600
#CPPFLAGS+=-DARDUINO_AVR_PRO -DARDUINO_ARCH_AVR
#VARIANT=standard
#PROGRAMMER=arduino
include ../lib.mk
CPPFLAGS+=-Wall -Wextra -I. -Os -fno-exceptions -ffunction-sections -fdata-sections
mega.elf: mega.o DS1302.o libDallasTemperature.a libOneWire.a libSD.a libSPI.a libcore.a
main.elf: main.o DS1302.o libMemoryFree.a libDallasTemperature.a libOneWire.a libSD.a libUIPEthernet.a libSPI.a libcore.a
# For normal printf
# -Wl,-u,vfprintf -lprintf_flt
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

CardInfo.elf: libcore.a libSD.a libSPI.a

strings.elf: libcore.a 
