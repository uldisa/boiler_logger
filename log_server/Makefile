include ../Makefile

LIBS=core MemoryFree SD_ada SPI Ethernet DallasTemperature OneWire
#PORT=/dev/ttyACM0
PORT=COM11
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

#USER_LIB_PATH=$(CURDIR)/../libraries
WIRE_LIB_PATH=$(ARD_HOME)/hardware/arduino/avr/libraries/Wire
include ../lib.mk
ifeq ($(filter %-pc-cygwin,$(MAKE_HOST)),)
CPPFLAGS+=-I$(WIRE_LIB_PATH) -I$(WIRE_LIB_PATH)/utility
else
CPPFLAGS+=-I$(shell cygpath -m $(WIRE_LIB_PATH)) -I$(shell cygpath -m $(WIRE_LIB_PATH)/utility)
endif

libWire.a: Wire/Wire.o  Wire/twi.o
	$(AR) rcs $@ $^

Wire/Wire.o:$(WIRE_LIB_PATH)/Wire.cpp |$(CURDIR)/Wire/
	$(CXX_CMD)

Wire/twi.o:$(WIRE_LIB_PATH)/utility/twi.c |$(CURDIR)/Wire/
	$(CC_CMD)

$(CURDIR)/Wire/:
	mkdir -p $@


CPPFLAGS+=-Wall -Wextra -I. -Os -fno-exceptions -ffunction-sections -fdata-sections
main.elf: main.o twi.o TemperatureSensor.o DS1302.o LogRecord.o libMemoryFree.a libSD_ada.a libEthernet.a libSPI.a libcore.a libDallasTemperature.a libOneWire.a 
