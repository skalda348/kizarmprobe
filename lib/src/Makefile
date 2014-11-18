CPU  ?= cortex-m0
FLAGS = -c -mthumb -mcpu=$(CPU) -Os -ffunction-sections -fdata-sections
AS    = arm-none-eabi-as -mthumb
CC    = arm-none-eabi-gcc
CC   += $(FLAGS)
CXX   = arm-none-eabi-g++
CXX  += $(FLAGS) -fno-exceptions

ASRC_FILES=$(wildcard *.S)

OBJS   =
ifeq ($(CPU),cortex-m0)
OBJS   = $(ASRC_FILES:%.S=%.o)
endif
OBJS  += hack.o
OBJS  += heap.o
OBJS  += sscanf.o
OBJS  += newdel.o

DSTLIB = libprobe.a

%.o: %.S
	$(AS) $< -o $@
%.o: %.c
	$(CC) $< -o $@
%.o: %.cpp
	$(CXX) $< -o $@

$(DSTLIB): $(OBJS)
	ar rcs $(DSTLIB) $(OBJS)
	mv $(DSTLIB) ../



.PHONY: all clean

all: $(DSTLIB)

clean:
	-rm -f *.o *.a
