.ifdef ARMCM0
  .cpu cortex-m0
.endif
  .section .rodata
  .globl tdesc_cortex_m
  .globl tdesc_cortex_mf
  
  .globl stm32f1_xml_memory_map
  .globl stm32hd_xml_memory_map
  .globl stm32f1_flash_write_stub
  .globl stm32f1_flash_write_stub_size
  
  .globl stm32f4_xml_memory_map
  .globl stm32f4_flash_write_stub
  .globl stm32f4_flash_write_stub_size
  
tdesc_cortex_m:
  .incbin "./src/res/cm3regs.xml"
  .byte 0
tdesc_cortex_mf:
  .incbin "./src/res/cm3fregs.xml"
  .byte 0

stm32f1_xml_memory_map:
  .incbin "./src/res/stm32f1map.xml"
  .byte 0

stm32hd_xml_memory_map:
  .incbin "./src/res/stm32hdmap.xml"
  .byte 0

stm32f4_xml_memory_map:
  .incbin "./src/res/stm32f4map.xml"
  .byte 0

  .align 2
stm32f1_flash_write_stub:
  .incbin "./src/res/STM32F1.bin"
  /*
  Poslední word stubu určuje délku stubu, která je opravdu nutná pro funkci.
  Tedy bez toho přílepku adresy, bloku a dat, ktará se natáhnou až následně.
  */
stm32f1_flash_write_stub_size = . - 4

  .align 2
stm32f4_flash_write_stub:
  .incbin "./src/res/STM32F4.bin"
stm32f4_flash_write_stub_size = . - 4

