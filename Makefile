all: test.hex

#ARD_HOME=/cygdrive/c/PROGRA~1/Arduino
ARD_HOME=/home/uldisa/Arduino/build/linux/work

ARDMK_PATH=$(ARD_HOME)/hardware/tools/avr
CFLAGS=-mmcu=atmega2560 -DF_CPU=16000000L -DARDUINO=154 -DARDUINO_AVR_MEGA2560 -DARDUINO_ARCH_AVR
CXXFLAGS=-mmcu=atmega2560 -DF_CPU=16000000L -DARDUINO=154 -DARDUINO_AVR_MEGA2560 -DARDUINO_ARCH_AVR
LDFLAGS=-mmcu=atmega2560 -Wl,--gc-sections -lc -lm

CC_NAME      = avr-gcc
CXX_NAME     = avr-g++
OBJCOPY_NAME = avr-objcopy
OBJDUMP_NAME = avr-objdump
AR_NAME      = avr-ar
SIZE_NAME    = avr-size
NM_NAME      = avr-nm

# Names of executables
CC      = $(ARDMK_PATH)/bin/$(CC_NAME)
CXX     = $(ARDMK_PATH)/bin/$(CXX_NAME)
AS      = $(ARDMK_PATH)/bin/$(AS_NAME)
OBJCOPY = $(ARDMK_PATH)/bin/$(OBJCOPY_NAME)
OBJDUMP = $(ARDMK_PATH)/bin/$(OBJDUMP_NAME)
AR      = $(ARDMK_PATH)/bin/$(AR_NAME)
SIZE    = $(ARDMK_PATH)/bin/$(SIZE_NAME)
NM      = $(ARDMK_PATH)/bin/$(NM_NAME)
REMOVE  = rm -rf
MV      = mv -f
CAT     = cat
ECHO    = echo
MKDIR   = mkdir -p
CC_CMD=$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
CXX_CMD=$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
ifneq ($(filter %-pc-cygwin,$(MAKE_HOST)),)
CC_CMD=$(CC) $(CPPFLAGS) $(CFLAGS) -c `cygpath -m $<` -o $(foreach f,$@,`cygpath -m $(f)`)
CXX_CMD=$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c `cygpath -m $<` -o $(foreach f,$@,`cygpath -m $(f)`)
endif
%.cpp: %.ino
	echo '#line 1 "$<"' > $@
	echo '#include "Arduino.h"' >> $@
	echo 'void setup();' >> $@
	echo 'void loop();' >> $@
	echo '#line 1 ' >> $@
	cat $< >> $@	

%.elf: %.o 
	$(CC) $(LDFLAGS) -o $@ $^ -lc -lm

%.eep: %.elf
	$(OBJCOPY) -O ihex -j .eeprom $< $@ 

%.hex: %.elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@ 

%.prog: %.hex
ifeq ($(filter %-pc-cygwin,$(MAKE_HOST)),)
	$(ARDMK_PATH)/../avrdude -v -v -p atmega2560 -cwiring -P/dev/ttyACM1 -b115200 -U flash:w:$<:i -C $(ARDMK_PATH)/../avrdude.conf
else
	$(ARDMK_PATH)/bin/avrdude -v -v -p atmega2560 -cwiring -PCOM11 -b115200 -U flash:w:$<:i -C `cygpath -m $(ARDMK_PATH)/../etc/avrdude.conf`
endif

########################
LIBS=SD SPI WiFi
test.elf: libcore.a libSD.a libSPI.a
Blink.elf: libcore.a

print-%:
	@echo "$($(*))"
clean::
	rm -f $(foreach l,core $(LIBS),lib$(l).a)
	rm -Rf $(foreach l,core $(LIBS),$(l)/)
	rm -f *.o *.elf *.epp *.hex

OBJDIR=.
###### ALL Arduino core libraries
ARDUINO_CORE_PATH=$(ARD_HOME)/hardware/arduino/avr/cores/arduino
LIBcoreINCL=-I$(ARDUINO_CORE_PATH) -I$(ARDUINO_CORE_PATH)/../../variants/mega
ifneq ($(filter %-pc-cygwin,$(MAKE_HOST)),)
LIBcoreINCL=-I`cygpath -m $(ARDUINO_CORE_PATH)` -I`cygpath -m $(ARDUINO_CORE_PATH)/../../variants/mega`
endif

LIBcore_C_SRCS     = $(wildcard $(ARDUINO_CORE_PATH)/*.c)
LIBcore_CPP_SRCS   = $(wildcard $(ARDUINO_CORE_PATH)/*.cpp)

###### External libraries
ARD_LIB_PATH=$(ARD_HOME)/libraries

define get_lib_sources
LIB$1INCL=-I$2/src -I$2/arch/avr
ifneq ($(filter %-pc-cygwin,$(MAKE_HOST)),)
LIB$1INCL=-I`cygpath -m $2`/src -I`cygpath -m $2`/arch/avr
endif

LIB$1_C_SRCS     = $(foreach d,src src/utility arch/avr arch/avr/utility,$(wildcard $2/$(d)/*.c))
LIB$1_CPP_SRCS   = $(foreach d,src src/utility arch/avr arch/avr/utility,$(wildcard $2/$(d)/*.cpp))
endef
$(foreach l,$(LIBS),$(eval $(call get_lib_sources,$(l),$(ARD_LIB_PATH)/$(l))))

define cpp_compile_rules
LIB$1_OBJS       += $$(OBJDIR)/$1/$(notdir $(2:.cpp=.o))
$$(OBJDIR)/$1/$(notdir $(2:.cpp=.o)): $2 |$$(OBJDIR)/$1/
	$(value CXX_CMD)

endef
$(foreach l,core $(LIBS),$(foreach c,$(LIB$(l)_CPP_SRCS),$(eval $(call cpp_compile_rules,$(l),$(c)))))

define c_compile_rules
LIB$1_OBJS       += $$(OBJDIR)/$1/$(notdir $(2:.c=.o))
$$(OBJDIR)/$1/$(notdir $(2:.c=.o)): $2 |$$(OBJDIR)/$1/
	$(value CC_CMD)

endef
$(foreach l,core $(LIBS),$(foreach c,$(LIB$(l)_C_SRCS),$(eval $(call c_compile_rules,$(l),$(c)))))

$(addprefix $(OBJDIR)/,$(addsuffix /,core $(LIBS))):
	mkdir -p $@

define create_lib
lib$1.a:$$(LIB$1_OBJS)
	$$(AR) rcs $$@ $$^
endef
$(foreach l,core $(LIBS),$(eval $(call create_lib,$(l))))

CFLAGS+=$(foreach l,$(LIBS) core,$(LIB$lINCL))
CXXFLAGS+=$(foreach l,$(LIBS) core,$(LIB$lINCL))

