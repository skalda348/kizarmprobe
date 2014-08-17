#include <stdio.h>
#include <string.h>
#include "baselayer.h"
#include "stm32f1.h"
#include "resources.h"

/* Flash Program ad Erase Controller Register Map */
#define FPEC_BASE 0x40022000
#define FLASH_ACR       (FPEC_BASE+0x00)
#define FLASH_KEYR      (FPEC_BASE+0x04)
#define FLASH_OPTKEYR   (FPEC_BASE+0x08)
#define FLASH_SR        (FPEC_BASE+0x0C)
#define FLASH_CR        (FPEC_BASE+0x10)
#define FLASH_AR        (FPEC_BASE+0x14)
#define FLASH_OBR       (FPEC_BASE+0x1C)
#define FLASH_WRPR      (FPEC_BASE+0x20)

#define FLASH_CR_OBL_LAUNCH (1<<13)
#define FLASH_CR_OPTWRE     (1 << 9)
#define FLASH_CR_STRT       (1 << 6)
#define FLASH_CR_OPTER      (1 << 5)
#define FLASH_CR_OPTPG      (1 << 4)
#define FLASH_CR_MER        (1 << 2)
#define FLASH_CR_PER        (1 << 1)

#define FLASH_OBR_RDPRT     (1 << 1)

#define FLASH_SR_BSY        (1 << 0)

#define FLASH_OBP_RDP         0x1FFFF800
#define FLASH_OBP_RDP_KEY     0x5aa5
#define FLASH_OBP_RDP_KEY_F3  0x55AA

#define KEY1 0x45670123
#define KEY2 0xCDEF89AB

#define SR_ERROR_MASK 0x14
#define SR_EOP        0x20

#define DBGMCU_IDCODE     0xE0042000
#define DBGMCU_IDCODE_F0  0x40015800

static const char stm32f1_driver_str[] = "STM32, Medium density.";
static const char stm32hd_driver_str[] = "STM32, High density.";
static const char stm32f3_driver_str[] = "STM32F3xx";
static const char stm32f0_driver_str[] = "STM32F0xx";

STM32F1::STM32F1 (GdbServer *s, const char *name) : CortexMx (s, name),
  cErase  ("erase",  "Erase entire flash memory"),
  cOption ("option", "Manipulate option bytes") {
  cErase.setServer (s);
  cOption.setServer (s);
  addCmd (cErase);
  addCmd (cOption);

  pagesize = 0x400;
}

STM32F1::~STM32F1() {
}

void STM32F1::flash_unlock (void) {
  apdp.ap.ap_mem_write (FLASH_KEYR, KEY1);
  apdp.ap.ap_mem_write (FLASH_KEYR, KEY2);
}

int STM32F1::flash_erase (uint32_t addr, int len) {
  uint16_t sr;

  addr &= ~ (pagesize - 1);
  len  &= ~ (pagesize - 1);

  flash_unlock ();

  while (len) {
    /* Flash page erase instruction */
    apdp.ap.ap_mem_write (FLASH_CR, FLASH_CR_PER);
    /* write address to FMA */
    apdp.ap.ap_mem_write (FLASH_AR, addr);
    /* Flash page erase start instruction */
    apdp.ap.ap_mem_write (FLASH_CR, FLASH_CR_STRT | FLASH_CR_PER);

    /* Read FLASH_SR to poll for BSY bit */
    while (apdp.ap.ap_mem_read (FLASH_SR) & FLASH_SR_BSY)
      if (check_error ()) return -1;

    len  -= pagesize;
    addr += pagesize;
  }

  /* Check for error */
  sr = apdp.ap.ap_mem_read (FLASH_SR);
  if ( (sr & SR_ERROR_MASK) || ! (sr & SR_EOP))
    return -1;

  return 0;
}

int STM32F1::flash_write (uint32_t dest, const uint8_t *src, int len) {
  uint32_t offset = dest % 4;
  uint32_t words  = (offset + len + 3) / 4;
  uint32_t data[2 + words];

  /* Construct data buffer used by stub */
  data[0] = dest - offset;
  data[1] = words * 4;            /* length must always be a multiple of 4 */
  data[2] = 0xFFFFFFFF;           /* pad partial words with all 1s to avoid */
  data[words + 1] = 0xFFFFFFFF;   /* damaging overlapping areas */
  memcpy ( (uint8_t *) &data[2] + offset, src, len);

  /* Write stub and data to target ram and set PC */
  uint32_t base = 0x20000000;
  mem_write_words (base,  stm32f1_flash_write_stub, stm32f1_flash_write_stub_size);
  mem_write_words (base + stm32f1_flash_write_stub_size, data, len + 8);
  pc_write (base);
  if (check_error ())
    return -1;

  /* Execute the stub */
  halt_resume (0);
  while (!halt_wait ());

  /* Check for error */
  if (apdp.ap.ap_mem_read (FLASH_SR) & SR_ERROR_MASK)
    return -1;

  return 0;
}

int STM32F1::Handler (int argc, const char *argv[]) {
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
bool STM32F1::cmd_erase_mass (void) {
  debug ("%s\n", __func__);

  flash_unlock ();

  /* Flash mass erase start instruction */
  apdp.ap.ap_mem_write (FLASH_CR, FLASH_CR_MER);
  apdp.ap.ap_mem_write (FLASH_CR, FLASH_CR_STRT | FLASH_CR_MER);

  /* Read FLASH_SR to poll for BSY bit */
  while (apdp.ap.ap_mem_read (FLASH_SR) & FLASH_SR_BSY)
    if (check_error ())
      return false;

  /* Check for error */
  uint16_t sr = apdp.ap.ap_mem_read (FLASH_SR);
  if ( (sr & SR_ERROR_MASK) || ! (sr & SR_EOP))
    return false;

  return true;
}
bool STM32F1::cmd_option (int argc, const char *argv[]) {
  debug ("%s\n", __func__);

  uint32_t addr, val;
  uint32_t flash_obp_rdp_key;
  uint32_t rdprt;

  switch (idcode) {
    case 0x422:  /* STM32F30x */
    case 0x432:  /* STM32F37x */
    case 0x440:  /* STM32F0 */
      flash_obp_rdp_key = FLASH_OBP_RDP_KEY_F3;
      break;
    default:
      flash_obp_rdp_key = FLASH_OBP_RDP_KEY;
  }
  rdprt = (apdp.ap.ap_mem_read (FLASH_OBR) & FLASH_OBR_RDPRT);
  flash_unlock ();
  apdp.ap.ap_mem_write (FLASH_OPTKEYR, KEY1);
  apdp.ap.ap_mem_write (FLASH_OPTKEYR, KEY2);

  if ( (argc == 2) && !strcmp (argv[1], "erase")) {
    option_erase ();
    option_write_erased (FLASH_OBP_RDP, flash_obp_rdp_key);
  } else if (rdprt) {
    cOption.reply ("Device is Read Protected\n");
    cOption.reply ("Use \"monitor option erase\" to unprotect, erasing device\n");
    return true;
  } else if (argc == 3) {
    addr = strtol (argv[1], NULL, 0);
    val  = strtol (argv[2], NULL, 0);
    option_write (addr, val);
  } else {
    cOption.reply ("usage: monitor option erase\n");
    cOption.reply ("usage: monitor option <addr> <value>\n");
  }
  /*
    if (0 && flash_obp_rdp_key == FLASH_OBP_RDP_KEY_F3) {
      // Reload option bytes on F0 and F3
      val = apdp.ap.ap_mem_read (FLASH_CR);
      val |= FLASH_CR_OBL_LAUNCH;
      option_write (FLASH_CR, val);
      val &= ~FLASH_CR_OBL_LAUNCH;
      option_write (FLASH_CR, val);
    }
  */
  int i;
  for (i = 0; i < 0xf; i += 4) {
    addr = 0x1ffff800 + i;
    val = apdp.ap.ap_mem_read (addr);
    cOption.reply ("0x%08X: 0x%04X\n", addr, val & 0xFFFF);
    cOption.reply ("0x%08X: 0x%04X\n", addr + 2, val >> 16);
  }
  return true;
}
bool STM32F1::option_erase (void) {
  /* Erase option bytes instruction */
  apdp.ap.ap_mem_write (FLASH_CR, FLASH_CR_OPTER | FLASH_CR_OPTWRE);
  apdp.ap.ap_mem_write (FLASH_CR,
                        FLASH_CR_STRT | FLASH_CR_OPTER | FLASH_CR_OPTWRE);
  /* Read FLASH_SR to poll for BSY bit */
  while (apdp.ap.ap_mem_read (FLASH_SR) & FLASH_SR_BSY)
    if (check_error ())
      return false;
  return true;
}
bool STM32F1::option_write_erased (uint32_t addr, uint16_t value) {
  if (value == 0xffff)
    return true;
  /* Erase option bytes instruction */
  apdp.ap.ap_mem_write (FLASH_CR, FLASH_CR_OPTPG | FLASH_CR_OPTWRE);
  apdp.ap.ap_mem_write_halfword (addr, value);
  /* Read FLASH_SR to poll for BSY bit */
  while (apdp.ap.ap_mem_read (FLASH_SR) & FLASH_SR_BSY)
    if (check_error ())
      return false;
  return true;
}
bool STM32F1::option_write (uint32_t addr, uint16_t value) {
  uint16_t opt_val[8];
  int i, index;

  index = (addr - FLASH_OBP_RDP) / 2;
  if ( (index < 0) || (index > 7))
    return false;
  /* Retrieve old values */
  for (i = 0; i < 16; i = i +4) {
    uint32_t val = apdp.ap.ap_mem_read (FLASH_OBP_RDP + i);
    opt_val[i/2] = val & 0xffff;
    opt_val[i/2 +1] = val >> 16;
  }
  if (opt_val[index] == value)
    return true;
  /* Check for erased value */
  if (opt_val[index] != 0xffff)
    if (! (option_erase ()))
      return false;
  opt_val[index] = value;
  /* Write changed values*/
  for (i = 0; i < 8; i++)
    if (! (option_write_erased (FLASH_OBP_RDP + i*2,opt_val[i])))
      return false;
  return true;
}

bool STM32F1::probe (void) {
  bool res = CortexMx::probe ();
  if (!res) return res;
  // ...
  idcode = apdp.ap.ap_mem_read (DBGMCU_IDCODE);
  debug ("F1 idcode=0x%04X\n", idcode);
  switch (idcode & 0xfff) {
    case 0x410:  /* Medium density */
    case 0x412:  /* Low denisty */
    case 0x420:  /* Value Line, Low-/Medium density */
      setName (stm32f1_driver_str);
      xml_mem_map = stm32f1_xml_memory_map;
      return true;
    case 0x414:  /* High density */
    case 0x418:  /* Connectivity Line */
    case 0x428:  /* Value Line, High Density */
      pagesize = 0x800;
      setName (stm32hd_driver_str);
      xml_mem_map = stm32hd_xml_memory_map;
      return true;
    case 0x422:  /* STM32F30x */
    case 0x432:  /* STM32F37x */
      setName (stm32f3_driver_str);
      xml_mem_map = stm32hd_xml_memory_map;
      return true;
  }

  idcode = apdp.ap.ap_mem_read (DBGMCU_IDCODE_F0) & 0xfff;
  switch (idcode) {
    case 0x440:  /* STM32F0 */
      setName (stm32f0_driver_str);
      xml_mem_map = stm32f1_xml_memory_map;
      return true;
  }
  debug ("idcode=%08X\n", idcode);
  return false;
}
