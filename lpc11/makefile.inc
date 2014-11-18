VCOM = SERIAL
# VCOM je zatím velmí problematický virtuální sériový port.
# Takže odstanit komentář z 1.řádku lze jen na experimenty.
# Pro driver rom nefungují SETUP pakety pro nastavení parametrů
# linky. Pro driver usb by to už mělo chodit.

#API = rom
API = usb
# API určuje použitý driver USB stacku. Volby jsou
# rom nebo usb - rom je driver v čipu, usb je ze zdrojáků
# které byly na webu NXP. Dost bez záruky.


CROSS_COMPILE = arm-none-eabi-
LD_PATH   = ./$(PLATFORM)
LD_SCRIPT = -L$(ROOT_PATH)lib -L $(LD_PATH) -T lpc11u24.ld
CPU_TYPE  = cortex-m0
MFLAGS    = -mthumb -mcpu=$(CPU_TYPE)

ADEFINES = ARMCM0=1
LDFLAGS += -nostartfiles -nostdlib

LDLIBS += $(LD_SCRIPT)
LDLIBS += -lprobe

VPATH += ./$(PLATFORM)
VPATH += ./$(PLATFORM)/$(API)
VPATH += ./lib
INCLUDE_PATHS += -I ./$(PLATFORM)
INCLUDE_PATHS += -I ./$(PLATFORM)/$(API)
INCLUDE_PATHS += -I ./$(PLATFORM)/$(API)/api
INCLUDE_PATHS += -I ./cmsis/inc

# C++ files
OBJS += usbclass.o
# C files
OBJS  += system.o
OBJS  += startup.o

ifeq ($(API),usb)
OBJS  += usbcore.o
OBJS  += usbuser.o
OBJS  += usbhw.o
endif

#conditional - for dual USB interface

ifdef VCOM
DEFINES=-D$(VCOM)=1
OBJS  += comp_desc.o
OBJS  += usart1.o
else
OBJS  += usb_desc.o
endif
