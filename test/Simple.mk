all: test.hex

#ARD_HOME=/cygdrive/c/PROGRA~1/Arduino
ARD_HOME=/home/uldisa/Arduino/build/linux/work
ARD_CMD_HOME=$(ARD_HOME)
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
%.elf: %.o 
	$(CC) $(LDFLAGS) -o $@ $^ -lc -lm


OBJDIR=.
###### ALL Arduino core libraries
ARDUINO_CORE_PATH=$(ARD_HOME)/hardware/arduino/avr/cores/arduino
COREINCL=-I$(ARD_CMD_HOME)/hardware/arduino/avr/cores/arduino -I$(ARD_CMD_HOME)/hardware/arduino/avr/variants/mega

CORE_C_SRCS     = $(wildcard $(ARDUINO_CORE_PATH)/*.c)
CORE_CPP_SRCS   = $(wildcard $(ARDUINO_CORE_PATH)/*.cpp)

CORE_OBJ_FILES  = $(CORE_C_SRCS:.c=.o) $(CORE_CPP_SRCS:.cpp=.o)
CORE_OBJS       = $(addprefix $(OBJDIR)/,$(notdir $(CORE_OBJ_FILES)))

libcore.a:	$(CORE_OBJS)
		$(AR) rcs $@ $<

$(CORE_OBJS):CFLAGS+=$(COREINCL)
$(CORE_OBJS):CXXFLAGS+=$(COREINCL)

$(OBJDIR)/%.o: $(ARDUINO_CORE_PATH)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(ARDUINO_CORE_PATH)/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
###### External libraries
LIBS=SD SPI
ARD_LIB_PATH=$(ARD_CMD_HOME)/libraries

define get_lib_sources
LIB$1INCL=-I$2/src -I$2/arch/avr
OBJDIR$1=$(OBJDIR)/$1

LIB$1_C_SRCS     = $(foreach d,src src/utility arch/avr arch/avr/utility,$(wildcard $2/$(d)/*.c))
LIB$1_CPP_SRCS   = $(foreach d,src src/utility arch/avr arch/avr/utility,$(wildcard $2/$(d)/*.cpp))
endef
$(foreach l,$(LIBS),$(eval $(call get_lib_sources,$(l),$(ARD_LIB_PATH)/$(l))))

define ard_lib

LIB$1_OBJ_FILES  = $$(LIB$1_C_SRCS:.c=.o) $$(LIB$1_CPP_SRCS:.cpp=.o)
LIB$1_OBJS       = $$(addprefix $$(OBJDIR$1)/,$$(notdir $$(LIB$1_OBJ_FILES)))

lib$1.a:	$$(LIB$1_OBJS)
		$$(AR) rcs $$@ $$^

$$(LIB$1_OBJS):CFLAGS+=$$(COREINCL) $$(LIB$1INCL)
$$(LIB$1_OBJS):CXXFLAGS+=$$(COREINCL) $$(LIB$1INCL)

#$$(OBJDIR$1)/%.o: $2/src/%.c |$$(OBJDIR$1)/
#	$$(CC) $$(CPPFLAGS) $$(CFLAGS) -c $$< -o $$@
#
#$$(OBJDIR$1)/%.o: $2/arch/avr/%.c |$$(OBJDIR$1)/
#	$$(CXX) $$(CPPFLAGS) $$(CXXFLAGS) -c $$< -o $$@
#
#$$(OBJDIR$1)/%.o: $2/src/%.cpp  |$$(OBJDIR$1)/
#	$$(CXX) $$(CPPFLAGS) $$(CXXFLAGS) -c $$< -o $$@
#
#$$(OBJDIR$1)/%.o: $2/arch/avr/%.cpp |$$(OBJDIR$1)/
#	$$(CXX) $$(CPPFLAGS) $$(CXXFLAGS) -c $$< -o $$@

endef

define c_to_o
$2: $1 |$(dir $2) 
	$$(CC) $$(CPPFLAGS) $$(CFLAGS) -c $$< -o $$@

endef

$(info aaaa $(LIBSD_OBJS))
$(foreach l,$(LIBS),$(eval $(call ard_lib,$(l),$(ARD_LIB_PATH)/$(l))))
$(info aaaa $(LIBSD_OBJS))
########################

#%.o: %.c
#	avr-gcc -mmcu=atmega2560 -DF_CPU=16000000L -DARDUINO=154 test.c -o test.o

CFLAGS=-mmcu=atmega2560 -DF_CPU=16000000L -DARDUINO=154
CXXFLAGS=-mmcu=atmega2560 -DF_CPU=16000000L -DARDUINO=154
test.o: CXXFLAGS+=$(COREINCL) $(LIBSDINCL) $(LIBSPIINCL)
#%.o: %.cpp
#	avr-g++ -mmcu=atmega2560 -DF_CPU=16000000L -DARDUINO=154 test.c -o test.o

test.elf: libcore.a libSD.a
test.elf: LDFLAGS+=-mmcu=atmega2560 -Wl,--gc-sections -lc -lm
prog: test.hex
	avrdude -p atmega2560 -c arduino -P \\\.\COM11 -U flash:w:test.hex:i -C D:\Progra~1\arduino-1.0-beta1/hardware/tools/avr/etc/avrdude.conf

print-%:
	@echo "$($(*))"
clean:
	rm -f *.o libcore.a 
