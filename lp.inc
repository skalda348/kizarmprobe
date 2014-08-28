# VCOM = SERIAL
# VCOM je zatím velmí problematický virtuální sériový port.
# Takže odstanit komentář z 1.řádku lze jen na experimenty.

CROSS_COMPILE = arm-none-eabi-
LD_PATH   = $(ROOT_PATH)lib
LD_SCRIPT = -L $(LD_PATH) -T lpc11u24.ld
CPU_TYPE  = cortex-m0
MFLAGS    = -mthumb -mcpu=$(CPU_TYPE)

ADEFINES = ARMCM0=1
LDFLAGS += -nostartfiles -nostdlib

LDLIBS += $(LD_SCRIPT)
LDLIBS += -lprobe

VPATH += ./lpc11
VPATH += ./lib
INCLUDE_PATHS += -I ./lpc11
INCLUDE_PATHS += -I ./lpc11/api
INCLUDE_PATHS += -I ./cmsis/inc

# C++ files
OBJS += usbclass.o
# C files
OBJS  += system.o
OBJS  += startup.o

#conditional - dual USB interface
ifdef VCOM
DEFINES=-D$(VCOM)=1
OBJS  += comp_desc.o
OBJS  += usart1.o
else
OBJS  += usb_desc.o
endif
