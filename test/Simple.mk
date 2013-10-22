all: test.hex

ARD_HOME=/cygdrive/c/PROGRA~1/Arduino
ARD_CMD_HOME=ARD_HOME
ifneq ($(filter %-pc-cygwin,$(MAKE_HOST)),)
ARD_CMD_HOME=c:/PROGRA~1/Arduino
endif

ARDMK_PATH=$(ARD_HOME)/hardware/tools/avr/bin

CC_NAME      = avr-gcc
CXX_NAME     = avr-g++
OBJCOPY_NAME = avr-objcopy
OBJDUMP_NAME = avr-objdump
AR_NAME      = avr-ar
SIZE_NAME    = avr-size
NM_NAME      = avr-nm

# Names of executables
CC      = $(ARDMK_PATH)/$(CC_NAME)
CXX     = $(ARDMK_PATH)/$(CXX_NAME)
AS      = $(ARDMK_PATH)/$(AS_NAME)
OBJCOPY = $(ARDMK_PATH)/$(OBJCOPY_NAME)
OBJDUMP = $(ARDMK_PATH)/$(OBJDUMP_NAME)
AR      = $(ARDMK_PATH)/$(AR_NAME)
SIZE    = $(ARDMK_PATH)/$(SIZE_NAME)
NM      = $(ARDMK_PATH)/$(NM_NAME)
REMOVE  = rm -rf
MV      = mv -f
CAT     = cat
ECHO    = echo
MKDIR   = mkdir -p

%.hex: %.o
	$(OBJCOPY) -O ihex test.o test.hex

SYSLIBS=SD SPI
SYSINCL=-I$(ARD_CMD_HOME)/hardware/arduino/avr/cores/arduino -I$(ARD_CMD_HOME)/hardware/arduino/avr/variants/mega
SYSINCL+=$(addprefix -I$(ARD_CMD_HOME)/libraries/,$(addsuffix /src,$(SYSLIBS)))
SYSINCL+=$(addprefix -I$(ARD_CMD_HOME)/libraries/,$(addsuffix /arch/avr,$(SYSLIBS)))

#%.o: %.c
#	avr-gcc -mmcu=atmega2560 -DF_CPU=16000000L -DARDUINO=154 test.c -o test.o

CFLAGS=-mmcu=atmega2560 -DF_CPU=16000000L -DARDUINO=154
CXXFLAGS=-mmcu=atmega2560 -DF_CPU=16000000L -DARDUINO=154
test.o: CXXFLAGS+=$(SYSINCL)
#%.o: %.cpp
#	avr-g++ -mmcu=atmega2560 -DF_CPU=16000000L -DARDUINO=154 test.c -o test.o

prog: test.hex
	avrdude -p atmega2560 -c arduino -P \\\.\COM11 -U flash:w:test.hex:i -C D:\Progra~1\arduino-1.0-beta1/hardware/tools/avr/etc/avrdude.conf
clean:
	rm test.o
	rm test.hex
