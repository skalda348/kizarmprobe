#include "cdclass.h"
#include "packet.h"
#include "swdp.h"

/**
 * @file
 * Debug firmware pro SWD je docela jednoduchý - využijeme toho, co už je hotovo,
 * přidáme mezivrstvu Packet a máme hotovo. Princip je popsán v main.h
 */

static CDClass l1 (& iAssoc0);
static Packet  l2;
static Swdp    l3;

int main (void) {
  l3 += l2 += l1;
  l1.Init();
  for (;;);
  return 0;
}
