/*
 * LPC11u24 linker script file.
 * This linker is intended to use with libc.
 */

MEMORY {
  irom  (rx)      : ORIGIN = 0x0,         LENGTH = 0x8000  /* 32k  */
  isram (rwx)     : ORIGIN = 0x10000000,  LENGTH = 0x2000  /* 8k   */
  isramUsb (rwx)  : ORIGIN = 0x20004000,  LENGTH = 0x800   /* 2k   */
}

  /* Define a symbol for the top of stack */
  /* It won't be used if user define FIXED_STACKHEAP_SIZE */
  _vStackTop = ORIGIN(isram) + LENGTH(isram);

ENTRY(ResetISR)

  VectorCheckSum = 0 - (_vStackTop + ResetISR + NMI_Handler + HardFault_Handler + 3);

SECTIONS {
  /* MAIN TEXT SECTION */
  .text : ALIGN(4)  {
    _text = .;
    /* Always keep this vector table */
    KEEP(*(.isr_vector))
    *(.after_vectors*)
    *(.text*)
    *(.rodata*)
    /* Pro použití statických konstruktorů v C++, KEEP musí být použit při gc */
    . = ALIGN(4);
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end   = .);
    
    . = ALIGN(4);
    _etext = .;
  } > irom

  _sidata = .;

  /* MAIN DATA SECTION */
  .data : AT (_sidata) {
    . = ALIGN (4);
    _data = .;
    *(.data*)
    . = ALIGN(4) ;
    _edata = .;
  } > isram

    . = ALIGN (4);
  /* MAIN BSS SECTION */
  .bss : {
    _bss = .;
    *(.bss)
    *(.bss*)
    . = ALIGN (4);
  } > isram
  .common (NOLOAD) : {
    *(.heap)
    *(COMMON)
    __fixed_heap_end = .;
    . = ALIGN(4) ;
    _ebss = .;
    PROVIDE(end = .);
  } > isram




  /* This is used for USB RAM section */
  .usb_ram (NOLOAD): ALIGN(4)  {
    *(USB_RAM)
  } > isramUsb
  /* Remove information from the standard libraries */
  /DISCARD/ : {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
