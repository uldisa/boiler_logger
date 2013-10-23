all: test.hex

ARD_HOME=/cygdrive/c/PROGRA~1/Arduino
#ARD_HOME=/home/uldisa/Arduino/build/linux/work
ARD_CMD_HOME=$(ARD_HOME)
ifneq ($(filter %-pc-cygwin,$(MAKE_HOST)),)
ARD_CMD_HOME=c:/PROGRA~1/Arduino
endif

ARDMK_PATH=$(ARD_HOME)/hardware/tools/avr/bin
CFLAGS=-mmcu=atmega2560 -DF_CPU=16000000L -DARDUINO=154
CXXFLAGS=-mmcu=atmega2560 -DF_CPU=16000000L -DARDUINO=154

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
CC_CMD=$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
CXX_CMD=$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
ifneq ($(filter %-pc-cygwin,$(MAKE_HOST)),)
CC_CMD=$(CC) $(CPPFLAGS) $(CFLAGS) -c `cygpath -m $<` -o $(foreach f,$@,`cygpath -m $(f)`)
CXX_CMD=$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c `cygpath -m $<` -o $(foreach f,$@,`cygpath -m $(f)`)
endif
%.hex: %.elf
	$(OBJCOPY) -O ihex $< $@ 
%.elf: %.o 
	$(CC) $(LDFLAGS) -o $@ $^ -lc -lm
%.prog: %.hex
ifeq ($(filter %-pc-cygwin,$(MAKE_HOST)),)
	$(ARDMK_PATH)/avrdude -v -v -p atmega2560 -cwiring -PCOM11 -b115200 -U flash:w:$<:i -C $(ARDMK_PATH)/../etc/avrdude.conf
else
	$(ARDMK_PATH)/avrdude -v -v -p atmega2560 -cwiring -PCOM11 -b115200 -U flash:w:$<:i -C `cygpath -m $(ARDMK_PATH)/../etc/avrdude.conf`
endif


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
LIBS=SD SPI WiFi
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

#$$(LIB$1_OBJS):CFLAGS+=$$(COREINCL) $$(LIB$1INCL)
#$$(LIB$1_OBJS):CXXFLAGS+=$$(COREINCL) $$(LIB$1INCL)

CFLAGS+=$(foreach l,$(LIBS) core,$(LIB$lINCL))
CXXFLAGS+=$(foreach l,$(LIBS) core,$(LIB$lINCL))

$(info aaaa $(LIBSD_OBJS))
#$(foreach l,$(LIBS),$(eval $(call ard_lib,$(l),$(ARD_LIB_PATH)/$(l))))
$(info aaaa $(LIBSD_OBJS))
########################


test.elf: libcore.a libSD.a libSPI.a
test.elf: LDFLAGS+=-mmcu=atmega2560 -Wl,--gc-sections -lc -lm

print-%:
	@echo "$($(*))"
clean:
	rm -f *.o libcore.a 
