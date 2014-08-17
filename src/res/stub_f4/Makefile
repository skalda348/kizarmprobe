OUT = STM32F4
CROSS = arm-none-eabi-
AS  = $(CROSS)as
LD  = $(CROSS)ld
CP  = $(CROSS)objcopy

OBJS = stm32.o

all: $(OUT).elf

$(OUT).elf: $(OBJS)
	$(LD) $(OBJS) -T ../stm.ld -o $(OUT).elf
	$(CP) -O binary $(OUT).elf $(OUT).bin
	mv $(OUT).bin ../

%.o: %.s
	$(AS) -adnlhns=stm.lst -o $@ $<

clean:
	rm -f $(OUT).elf $(OUT).bin *.lst *.o *~