#include <stdio.h>
#include <string.h>
#include "baselayer.h"
#include "resources.h"
#include "stm32f4.h"

/* Flash Program ad Erase Controller Register Map */
#define FPEC_BASE     0x40023C00
#define FLASH_ACR     (FPEC_BASE+0x00)
#define FLASH_KEYR    (FPEC_BASE+0x04)
#define FLASH_OPTKEYR (FPEC_BASE+0x08)
#define FLASH_SR      (FPEC_BASE+0x0C)
#define FLASH_CR      (FPEC_BASE+0x10)
#define FLASH_OPTCR   (FPEC_BASE+0x14)

#define FLASH_CR_PG       (1 << 0)
#define FLASH_CR_SER      (1 << 1)
#define FLASH_CR_MER      (1 << 2)
#define FLASH_CR_PSIZE8   (0 << 8)
#define FLASH_CR_PSIZE16  (1 << 8)
#define FLASH_CR_PSIZE32  (2 << 8)
#define FLASH_CR_PSIZE64  (3 << 8)
#define FLASH_CR_STRT     (1 << 16)
#define FLASH_CR_EOPIE    (1 << 24)
#define FLASH_CR_ERRIE    (1 << 25)
#define FLASH_CR_STRT     (1 << 16)
#define FLASH_CR_LOCK     (1 << 31)

#define FLASH_SR_BSY        (1 << 16)

#define FLASH_OPTCR_OPTLOCK (1 << 0)
#define FLASH_OPTCR_OPTSTRT (1 << 1)
#define FLASH_OPTCR_RESERVED 0xf0000013

#define KEY1 0x45670123
#define KEY2 0xCDEF89AB

#define OPTKEY1 0x08192A3B
#define OPTKEY2 0x4C5D6E7F

#define SR_ERROR_MASK 0xF2
#define SR_EOP        0x01

#define DBGMCU_IDCODE 0xE0042000

static const char stm32f4_driver_str[] = "STM32F4xx";

STM32F4::STM32F4 (GdbServer* s, const char* name) : CortexMx (s, name),
  cErase  ("erase",  "Erase entire flash memory"),
  cOption ("option", "Manipulate option bytes") {
  cErase.setServer (s);
  cOption.setServer (s);
  addCmd (cErase);
  addCmd (cOption);
  
  pagesize = 0x400;
}

STM32F4::~STM32F4() {
}


int STM32F4::Handler (int argc, const char* argv []) {
  int res = CortexMx::Handler (argc, argv);
  switch (res) {
    case 2:
      cmd_erase_mass ();
      break;
    case 3:
      cmd_option (argc, argv);
      break;
    default:
      break;
  }
  return res;
}

bool STM32F4::probe (void) {
  bool res = CortexMx::probe ();
  if (!res) return res;
  
  idcode = apdp.ap.ap_mem_read (DBGMCU_IDCODE);
  debug ("F4 idcode=0x%04X\n", idcode);
  switch (idcode & 0xFFF) {
  case 0x411: /* Documented to be 0x413! This is what I read... */
  case 0x413: /* STM32F405xx/07xx and STM32F415xx/17xx          */
  case 0x423: /* F401 */
  case 0x419: /* 427/437 */
    setName (stm32f4_driver_str);
    xml_mem_map = stm32f4_xml_memory_map;
    return true;
  }
  return false;
}

void STM32F4::flash_unlock (void) {

  if (apdp.ap.ap_mem_read(FLASH_CR) & FLASH_CR_LOCK) {
    /* Enable FPEC controller access */
    apdp.ap.ap_mem_write (FLASH_KEYR, KEY1);
    apdp.ap.ap_mem_write (FLASH_KEYR, KEY2);
  }
}

int STM32F4::flash_erase (uint32_t addr, int len) {
  uint16_t sr;
  uint32_t cr;

  addr &= 0x07FFC000;

  flash_unlock ();

  while (len) {
    if (addr < 0x10000) {       /* Sector 0..3 */
      cr = (addr >> 11);
      pagesize = 0x4000;
    } else if (addr < 0x20000) { /* Sector 4 */
      cr = (4 << 3);
      pagesize = 0x10000;
    } else if (addr < 0x100000) { /* Sector 5..11 */
      cr = (((addr - 0x20000) >> 14) + 0x28);
      pagesize = 0x20000;
    } else { /* Sector > 11 ?? */
      return -1;
    }
    cr |= FLASH_CR_EOPIE | FLASH_CR_ERRIE | FLASH_CR_SER;
    /* Flash page erase instruction */
    apdp.ap.ap_mem_write (FLASH_CR, cr);
    /* write address to FMA */
    apdp.ap.ap_mem_write (FLASH_CR, cr | FLASH_CR_STRT);

    /* Read FLASH_SR to poll for BSY bit */
    while (apdp.ap.ap_mem_read (FLASH_SR) & FLASH_SR_BSY)
      if (check_error()) return -1;

    len  -= pagesize;
    addr += pagesize;
  }

  /* Check for error */
  sr = apdp.ap.ap_mem_read (FLASH_SR);
  if (sr & SR_ERROR_MASK) return -1;

  return 0;
}

int STM32F4::flash_write (uint32_t dest, const uint8_t* src, int len) {
  uint32_t offset = dest % 4;
  uint32_t words = (offset + len + 3) / 4;
  uint32_t data [2 + words];
  uint16_t sr;

  /* Construct data buffer used by stub */
  data[0] = dest - offset;
  data[1] = words * 4;    /* length must always be a multiple of 4 */
  data[2] = 0xFFFFFFFF;   /* pad partial words with all 1s to avoid */
  data[words + 1] = 0xFFFFFFFF; /* damaging overlapping areas */
  memcpy ((uint8_t *) & data[2] + offset, src, len);

  const uint32_t base = 0x20000000;
  /* Write stub and data to target ram and set PC */
  mem_write_words (base,  stm32f4_flash_write_stub, stm32f4_flash_write_stub_size);
  mem_write_words (base + stm32f4_flash_write_stub_size, data, sizeof (data));
  pc_write        (0x20000000);
  if (check_error ()) return -1;

  /* Execute the stub */
  halt_resume(0);
  while (!halt_wait());

  /* Check for error */
  sr = apdp.ap.ap_mem_read (FLASH_SR);
  if(sr & SR_ERROR_MASK)
    return -1;

  return 0;
}

bool STM32F4::cmd_erase_mass (void) {

  cErase.reply ("Erasing flash... This may take a few seconds.  ");
  flash_unlock ();

  /* Flash mass erase start instruction */
  apdp.ap.ap_mem_write (FLASH_CR, FLASH_CR_MER);
  apdp.ap.ap_mem_write (FLASH_CR, FLASH_CR_STRT | FLASH_CR_MER);

  /* Read FLASH_SR to poll for BSY bit */
  while (apdp.ap.ap_mem_read (FLASH_SR) & FLASH_SR_BSY) {
    if (check_error ()) {
      cErase.reply ("Error\n");
      return false;
    }
  }
  cErase.reply ("End\n");

  /* Check for error */
  uint16_t sr = apdp.ap.ap_mem_read (FLASH_SR);
  if ((sr & SR_ERROR_MASK) || !(sr & SR_EOP))
    return false;

  return true;
}

bool STM32F4::option_write (uint32_t value) {

  apdp.ap.ap_mem_write (FLASH_OPTKEYR, OPTKEY1);
  apdp.ap.ap_mem_write (FLASH_OPTKEYR, OPTKEY2);
  value &= ~FLASH_OPTCR_RESERVED;
  while (apdp.ap.ap_mem_read (FLASH_SR) & FLASH_SR_BSY)
    if (check_error()) return -1;

  /* WRITE option bytes instruction */
  apdp.ap.ap_mem_write (FLASH_OPTCR, value);
  apdp.ap.ap_mem_write (FLASH_OPTCR, value | FLASH_OPTCR_OPTSTRT);
  /* Read FLASH_SR to poll for BSY bit */
  while (apdp.ap.ap_mem_read (FLASH_SR) & FLASH_SR_BSY)
    if (check_error()) return false;
  apdp.ap.ap_mem_write (FLASH_OPTCR, value | FLASH_OPTCR_OPTLOCK);
  return true;
}

bool STM32F4::cmd_option (int argc, const char* argv[]) {
  uint32_t addr, val;

  if ((argc == 2) && !strcmp(argv[1], "erase")) {
    option_write (0x0fffaaed);
  }
  else if ((argc == 3) && !strcmp(argv[1], "write")) {
    val = strtoul (argv[2], NULL, 0);
    option_write  (val);
  } else {
    cOption.reply ("usage: monitor option erase\n");
    cOption.reply ("usage: monitor option write <value>\n");
  }
  int i;
  for (i = 0; i < 0xf; i += 8) {
    addr = 0x1fffC000 + i;
    val = apdp.ap.ap_mem_read (addr);
    cOption.reply ("0x%08X: 0x%04X\n", addr, val & 0xFFFF);
  }
  return true;

}
/*
bool STM32F4::option_write_erased (uint32_t addr, uint16_t value) {

}

bool STM32F4::option_erase (void) {

}
*/
