##########################################################################
# User configuration and firmware specific object files	
##########################################################################
ROOT_PATH = ./
INCLUDE_PATHS  = -I. -I../lpc11
INCLUDE_PATHS += -I ../lpc11/api
INCLUDE_PATHS += -I ../cmsis/inc

MFLAGS  = -mthumb -mcpu=cortex-m0
LDFLAGS = $(MFLAGS)
LDFLAGS+= -L ../lib -T lpc11u24.ld
LDFLAGS+= -nostartfiles -nostdlib
LDLIBS  = -laeabi-cortexm0
CFLAGS  =

VPATH = . ../lpc11
OBJS  = 

CFLAGS  = $(MFLAGS)
CFLAGS += $(DEFINES)

# C files
OBJS+= debug.o

# C++ files
OBJS+= socket.o
OBJS+= swdp.o
OBJS+= packet.o
OBJS+= usbclass.o
# C files
OBJS  += system.o
OBJS  += startup.o
OBJS  += usb_desc.o

##########################################################################
# GNU GCC compiler prefix and location
##########################################################################
CROSS_COMPILE = arm-none-eabi-

AS  = $(CROSS_COMPILE)as
CC  = $(CROSS_COMPILE)gcc
LD  = $(CROSS_COMPILE)g++
CCP = $(CROSS_COMPILE)g++
SIZE = $(CROSS_COMPILE)size
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
OUTFILE = firmware

##########################################################################
# Compiler settings, parameters and flags
##########################################################################

CFLAGS += -c -Os $(INCLUDE_PATHS) -Wall
CFLAGS += -Wa,-adhlns=$(@:%.o=%.lst)
# Umístit kód funkcí i proměnné do vzláštních sekcí
CFLAGS += -ffunction-sections -fdata-sections

LDFLAGS+= -Wl,--gc-sections,-Map=$(OUTFILE).map,--cref
OCFLAGS = --strip-unneeded

CPFLAGS = $(CFLAGS) -g
CPFLAGS+= -fno-rtti -fno-exceptions

all: $(OUTFILE).elf

%.o : %.c
	$(CC) $(CFLAGS) -o $@ $<

%.o : %.cpp
	$(CCP) $(CPFLAGS) -o $@ $<


$(OUTFILE).elf: $(OBJS)
	$(LD) $(LDFLAGS) -o $(OUTFILE).elf $(OBJS) $(LDLIBS)
	-@echo ""
	$(SIZE) -B $(OUTFILE).elf
	-@echo ""
	$(OBJCOPY) $(OCFLAGS) -O binary $(OUTFILE).elf $(OUTFILE).bin
	$(OBJCOPY) $(OCFLAGS) -O ihex $(OUTFILE).elf $(OUTFILE).hex
	$(OBJDUMP) -h -S $(OUTFILE).elf > $(OUTFILE).lst
	-@echo ""

clean:
	rm -f $(OBJS) $(OUTFILE).elf $(OUTFILE).bin $(OUTFILE).hex
	rm -f $(OUTFILE).map *~ *.lst
	$(foreach XPATH, $(VPATH), rm -f $(XPATH)/*~ )
