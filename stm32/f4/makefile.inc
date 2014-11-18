
CROSS_COMPILE = arm-none-eabi-
LD_PATH   = ./$(PLATFORM)
#LD_SCRIPT = -L$(ROOT_PATH)lib -L $(LD_PATH) -T stm32_ram.ld
LD_SCRIPT = -L$(ROOT_PATH)lib -L $(LD_PATH) -T stm32_flash.ld
CPU_TYPE  = cortex-m4
MFLAGS    = -mthumb -mcpu=$(CPU_TYPE)

ADEFINES = ARMCM0=1
LDFLAGS += -nostartfiles -nostdlib

LDLIBS += $(LD_SCRIPT)
LDLIBS += -lprobe

VPATH += ./$(PLATFORM)
VPATH += ./$(PLATFORM)/usb/src
VPATH += ./lib
INCLUDE_PATHS += -I ./$(PLATFORM)
INCLUDE_PATHS += -I ./$(PLATFORM)/usb/inc
INCLUDE_PATHS += -I ./cmsis/inc

# C++ files
# C files
OBJS  += system.o
OBJS  += startup.o
OBJS  += gpio.o

OBJS += irqs.o
OBJS += usb_bsp.o
OBJS += usb_core.o
OBJS += usb_dcd.o
OBJS += usb_dcd_int.o

OBJS += usbd_cdc_core.o
OBJS += usbd_core.o
OBJS += usbd_desc.o
OBJS += usbd_ioreq.o
OBJS += usbd_req.o
OBJS += usbd_usr.o

