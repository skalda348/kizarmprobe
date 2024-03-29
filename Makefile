##########################################################################
# User configuration and firmware specific object files	
##########################################################################
ROOT_PATH = ./
INCLUDE_PATHS  = -I. -I./inc
LDFLAGS =
LDLIBS  =
CFLAGS  =

VPATH = .
OBJS  = 
# supported platforms lpc11, i386
PLATFORM ?= lpc11
#PLATFORM ?= i386
# experimental on STM32F4 Discovery
#PLATFORM ?= stm32/f4

include ./$(PLATFORM)/makefile.inc

LDFLAGS+= $(MFLAGS)
CFLAGS += $(MFLAGS)
CFLAGS += $(DEFINES)

VPATH += ./src
# C files
OBJS+= resources.o

# AS files
OBJS+= utils.o

# C++ files
OBJS+= main.o
OBJS+= cdclass.o
OBJS+= gdbpacket.o
OBJS+= gdbserver.o
OBJS+= target.o
OBJS+= monitor.o

OBJS+= swdp.o
OBJS+= adiv5apdp.o
OBJS+= cortexmx.o
OBJS+= stm32f1.o
OBJS+= stm32f4.o
OBJS+= lpc11xx.o

OBJS+= command.o
OBJS+= commandset.o


##########################################################################
# GNU GCC compiler prefix and location
##########################################################################

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

all: $(OUTFILE).elf #./dbg/$(OUTFILE).elf

./dbg/$(OUTFILE).elf: ./lib/libprobe.a
	cd ./dbg && make all
./lib/libprobe.a:
	cd ./lib/src && make CPU=$(CPU_TYPE) all

%.o : %.c
	$(CC) $(CFLAGS) -o $@ $<

%.o : %.cpp
	$(CCP) $(CPFLAGS) -o $@ $<

%.o : %.s
	$(AS) --defsym $(ADEFINES) -o $@ $<

$(OUTFILE).elf: ./lib/libprobe.a $(OBJS)
	$(LD) $(LDFLAGS) -o $(OUTFILE).elf $(OBJS) $(LDLIBS)
	-@echo ""
	$(SIZE) -B $(OUTFILE).elf
	-@echo ""
	$(OBJCOPY) $(OCFLAGS) -O binary $(OUTFILE).elf $(OUTFILE).bin
	$(OBJCOPY) $(OCFLAGS) -O ihex $(OUTFILE).elf $(OUTFILE).hex
	$(OBJDUMP) -h -S $(OUTFILE).elf > $(OUTFILE).lst
	-@echo ""

clean:
	rm -f $(OBJS) $(LD_TEMP) $(OUTFILE).elf $(OUTFILE).bin $(OUTFILE).hex
	rm -f $(OUTFILE).map *~ *.lst
	rm -f $(foreach XPATH, $(VPATH),$(XPATH)/*~)
	rm -f  ./inc/*~
	cd ./dbg && make clean
