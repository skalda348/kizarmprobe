#include <stdio.h>
#include "main.h"
/** Pokud je to uděláno jako skládačka z vrstev BaseLayer, je spíš možné
 *  přizpůsobit to jinou platformu. Fakticky se mění jen první a poslední
 *  vrstva (l1, l4). GdbPacket a GdbServer zůstávají, GdbServer je nejsložitější,
 *  zapouzdřuje v sobě jednotlivé targety. 
 */
static CDClass   l1  (false, & iAssoc0);
static GdbPacket l2;
static GdbServer l3;
/** Tenhle poslední díl skládačky může být realizován také jako BaseLayer.
 * Není to sice úplně nezbytné, ale protože je to možné a ne příliš složité
 * nechť je to tak. Pak je jednoduší přizpůsobit to platformě.
 */
static Swdp      l4;

#ifdef SERIAL
static Mirror    top;
static CDClass   vcom (true, & iAssoc1);
static Usart1    serial (9600);
#endif
/// Tohle vyjmeme ze třídy - jen to by se mohlo měnit po přidání dalšího targetu.
void GdbServer::Scan (void) {
  OldTargetDestroy();                                         // jedeme nanovo
  if (probe (new STM32F1  (this, "STM32FXX")))      return;
  if (probe (new STM32F4  (this, "STM32F4X")))      return;
  if (probe (new LPC11XX  (this, "LPC11Xxx")))      return;
  if (probe (new CortexMx (this, "ARM Cortex-Mx"))) return;   // to je asi blbost
}

int main (void) {
  
  l4 += l3 += l2 += l1;
  l1.Init();

#ifdef SERIAL
  top += vcom;
  top += serial;
#endif

  for (;;) {
    l3.Polling ();
    if (!l1.Fini()) break;
  }
  // pro ARM celkem zbytecne
  l3.Fini ();
  l4.Fini ();
  return 0;
}
