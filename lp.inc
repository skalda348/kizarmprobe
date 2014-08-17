CROSS_COMPILE = arm-none-eabi-
LD_PATH   = $(ROOT_PATH)lib
LD_SCRIPT = -L $(LD_PATH) -T lpc11u24.ld
CPU_TYPE  = cortex-m0
MFLAGS    = -mthumb -mcpu=$(CPU_TYPE)

ADEFINES = ARMCM0=1
LDFLAGS += -nostartfiles -nostdlib
#LDFLAGS+= -specs=nano.specs

LDLIBS += $(LD_SCRIPT)
#LDLIBS += -L $(ROOT_PATH)lib
LDLIBS += -laeabi-cortexm0
#LDLIBS += -lc -lnosys -lstdc++

VPATH += ./lpc11
VPATH += ./lib
INCLUDE_PATHS += -I ./lpc11
INCLUDE_PATHS += -I ./lpc11/api
INCLUDE_PATHS += -I ./cmsis/inc

OBJS += newdel.o
OBJS += usbclass.o
# C files
OBJS  += system.o
OBJS  += startup.o
OBJS  += usb_desc.o

OBJS  += hack.o
OBJS  += heap.o
OBJS  += sscanf.o
