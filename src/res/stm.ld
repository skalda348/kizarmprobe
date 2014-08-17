MEMORY {
  ram  (rx)      : ORIGIN = 0x20000000,   LENGTH = 0x100  /* 256  */
}

ENTRY(_start)

SECTIONS {
  /* MAIN TEXT SECTION */
  .text : ALIGN(4)  {
    *(.text*)
  } > ram
}
