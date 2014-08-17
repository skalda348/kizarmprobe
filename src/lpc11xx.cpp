#include "lpc11xx.h"


#define MSP                           17         // Main stack pointer register number
#define MIN_RAM_SIZE_FOR_LPC1xxx      2048
#define RAM_USAGE_FOR_IAP_ROUTINES      32              // IAP routines use 32 bytes at top of ram

#define IAP_ENTRYPOINT  0x1fff1ff1
#define IAP_RAM_BASE    0x10000000


#define IAP_CMD_PREPARE     50
#define IAP_CMD_PROGRAM     51
#define IAP_CMD_ERASE       52
#define IAP_CMD_BLANKCHECK  53

#define IAP_STATUS_CMD_SUCCESS          0
#define IAP_STATUS_INVALID_COMMAND      1
#define IAP_STATUS_SRC_ADDR_ERROR       2
#define IAP_STATUS_DST_ADDR_ERROR       3
#define IAP_STATUS_SRC_ADDR_NOT_MAPPED  4
#define IAP_STATUS_DST_ADDR_NOT_MAPPED  5
#define IAP_STATUS_COUNT_ERROR          6
#define IAP_STATUS_INVALID_SECTOR       7
#define IAP_STATUS_SECTOR_NOT_BLANK     8
#define IAP_STATUS_SECTOR_NOT_PREPARED  9
#define IAP_STATUS_COMPARE_ERROR        10
#define IAP_STATUS_BUSY                 11

/*
 * Note that this memory map is actually for the largest of the lpc11xx devices;
 * There seems to be no good way to decode the part number to determine the RAM
 * and flash sizes.
 */
static const char lpc11xx_xml_memory_map[] = "<?xml version=\"1.0\"?>"
/*  "<!DOCTYPE memory-map "
  "             PUBLIC \"+//IDN gnu.org//DTD GDB Memory Map V1.0//EN\""
  "                    \"http://sourceware.org/gdb/gdb-memory-map.dtd\">"*/
  "<memory-map>"
  "  <memory type=\"flash\" start=\"0x00000000\" length=\"0x8000\">"
  "    <property name=\"blocksize\">0x1000</property>"
  "  </memory>"
  "  <memory type=\"ram\" start=\"0x10000000\" length=\"0x2000\"/>"
  "</memory-map>";

LPC11XX::LPC11XX (GdbServer* s, const char* name) : CortexMx (s, name) {

}

LPC11XX::~LPC11XX() {

}

bool LPC11XX::probe (void) {
  bool res = CortexMx::probe ();
  if (!res) return res;
  
  /* read the device ID register */
  idcode = apdp.ap.ap_mem_read (0x400483F4);

  debug ("IDCODE: %08X\n", idcode);
  switch (idcode) {

  case 0x041E502B:
  case 0x2516D02B:
  case 0x0416502B:
  case 0x2516902B:  /* lpc1111 */
  case 0x2524D02B:
  case 0x0425502B:
  case 0x2524902B:
  case 0x1421102B:  /* lpc1112 */
  case 0x0434502B:
  case 0x2532902B:
  case 0x0434102B:
  case 0x2532102B:  /* lpc1113 */
  case 0x0444502B:
  case 0x2540902B:
  case 0x0444102B:
  case 0x2540102B:
  case 0x1440102B:  /* lpc1114 */
  case 0x0A40902B:
  case 0x1A40902B:
  case 0x1431102B:  /* lpc11c22 */
  case 0x1430102B:  /* lpc11c24 */
  case 0x2980002B:  /* lpc11u24 */
  case 0x2988402B:
  case 0x2000002B:  /* lpc11u34 */
    setName ("LPC11xx");
    xml_mem_map = lpc11xx_xml_memory_map;
    return true;
  }

  return false;
}


void LPC11XX::iap_call (unsigned int param_len) {

  uint32_t regs [regs_size / 4];

  /* fill out the remainder of the parameters and copy the structure to RAM */
  flash_pgm.p.opcodes[0] = 0xbe00;
  flash_pgm.p.opcodes[1] = 0x0000;
  mem_write_words (IAP_RAM_BASE, (const uint32_t *) & flash_pgm.p, param_len);

  /* set up for the call to the IAP ROM */
  regs_read (regs);
  // offsetof nefunguje, tak to zkusime jinak
  const flash_param * temp = (flash_param * ) IAP_RAM_BASE;
  regs[0] = (uint32_t) & temp->command;
  regs[2] = (uint32_t) & temp->result;
//  regs[0] = IAP_RAM_BASE + offsetof (struct flash_param, command);
//  regs[1] = IAP_RAM_BASE + offsetof (struct flash_param, result);
  // stack pointer - top of the smallest ram less 32 for IAP usage
  regs[MSP] = IAP_RAM_BASE + MIN_RAM_SIZE_FOR_LPC1xxx - RAM_USAGE_FOR_IAP_ROUTINES;
  regs[14]  = IAP_RAM_BASE | 1;
  regs[15]  = IAP_ENTRYPOINT;
  regs_write (regs);

  /* start the target and wait for it to halt again */
  halt_resume (0);
  while (!halt_wait ());

  /* copy back just the parameters structure */
  mem_read_words ((uint32_t *) & flash_pgm.p, IAP_RAM_BASE, sizeof(struct flash_param));
}
extern "C" void *memset (void *s, int c, size_t n);
extern "C" void *memcpy (void *dest, const void *src, size_t n);

int LPC11XX::flash_prepare (uint32_t addr, int len) {

  /* prepare the sector(s) to be erased */
  memset (&flash_pgm.p, 0, sizeof(flash_pgm.p));
  flash_pgm.p.command[0] = IAP_CMD_PREPARE;
  flash_pgm.p.command[1] = addr / 4096;
  flash_pgm.p.command[2] = (addr + len - 1) / 4096;

  iap_call (sizeof (flash_pgm.p));
  if (flash_pgm.p.result[0] != IAP_STATUS_CMD_SUCCESS) {
    return -1;
  }

  return 0;
}

int LPC11XX::flash_erase (uint32_t addr, int len) {

  if (addr % 4096)
    return -1;

  /* prepare... */
  if (flash_prepare (addr, len))
    return -1;

  /* and now erase them */
  flash_pgm.p.command[0] = IAP_CMD_ERASE;
  flash_pgm.p.command[1] = addr / 4096;
  flash_pgm.p.command[2] = (addr + len - 1) / 4096;
  flash_pgm.p.command[3] = 12000; /* XXX safe to assume this? */
  iap_call (sizeof(flash_pgm.p));
  if (flash_pgm.p.result[0] != IAP_STATUS_CMD_SUCCESS) {
    return -1;
  }
  flash_pgm.p.command[0] = IAP_CMD_BLANKCHECK;
  iap_call (sizeof(flash_pgm.p));
  if (flash_pgm.p.result[0] != IAP_STATUS_CMD_SUCCESS) {
    return -1;
  }

  return 0;
}

int LPC11XX::flash_write (uint32_t dest, const uint8_t* src, int len) {
  unsigned first_chunk  =  dest / IAP_PGM_CHUNKSIZE;
  unsigned last_chunk   = (dest + len - 1) / IAP_PGM_CHUNKSIZE;
  unsigned chunk_offset =  dest % IAP_PGM_CHUNKSIZE;
  unsigned chunk;

  for (chunk = first_chunk; chunk <= last_chunk; chunk++) {

    debug ("chunk %u len %d\n", chunk, len);
    /* first and last chunk may require special handling */
    if ((chunk == first_chunk) || (chunk == last_chunk)) {

      /* fill with all ff to avoid sector rewrite corrupting other writes */
      memset(flash_pgm.data, 0xff, sizeof(flash_pgm.data));

      /* copy as much as fits */
      int copylen = IAP_PGM_CHUNKSIZE - chunk_offset;
      if (copylen > len)
        copylen = len;
      memcpy(&flash_pgm.data[chunk_offset], src, copylen);

      /* update to suit */
      len -= copylen;
      src += copylen;
      chunk_offset = 0;

      /* if we are programming the vectors, calculate the magic number */
      if (dest == 0) {
        uint32_t *w = (uint32_t *)(&flash_pgm.data[0]);
        uint32_t sum = 0;

        if (copylen >= 7) {
          unsigned i;
          for (i = 0; i < 7; i++)
            sum += w[i];
          w[7] = 0 - sum;
        } else {
          /* We can't possibly calculate the magic number */
          return -1;
        }
      }

    } else {

      /* interior chunk, must be aligned and full-sized */
      memcpy(flash_pgm.data, src, IAP_PGM_CHUNKSIZE);
      len -= IAP_PGM_CHUNKSIZE;
      src += IAP_PGM_CHUNKSIZE;
    }

    /* prepare... */
    if (flash_prepare (chunk * IAP_PGM_CHUNKSIZE, IAP_PGM_CHUNKSIZE))
      return -1;

    /* set the destination address and program */
    flash_pgm.p.command[0] = IAP_CMD_PROGRAM;
    flash_pgm.p.command[1] = chunk * IAP_PGM_CHUNKSIZE;
    const flash_program * temp = (flash_program * ) IAP_RAM_BASE;
    flash_pgm.p.command[2] = (uint32_t) & temp->data;
//    flash_pgm.p.command[2] = IAP_RAM_BASE + offsetof (struct flash_program, data);
    flash_pgm.p.command[3] = IAP_PGM_CHUNKSIZE;
    flash_pgm.p.command[4] = 12000; /* XXX safe to presume this? */
    iap_call (sizeof(flash_pgm));
    if (flash_pgm.p.result[0] != IAP_STATUS_CMD_SUCCESS) {
      return -1;
    }

  }

  return 0;
}

