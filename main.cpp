#include <stdio.h>
#include "main.h"
/** Pokud je to uděláno jako skládačka z vrstev BaseLayer, je spíš možné
 *  přizpůsobit to jinou platformu. Fakticky se mění jen první a poslední
 *  vrstva (l1, l4). GdbPacket a GdbServer zůstávají, GdbServer je nejsložitější,
 *  zapouzdřuje v sobě jednotlivé targety. 
 */
static Socket    l1;
static GdbPacket l2;
static GdbServer l3;
/** Tenhle poslední díl skládačky může být realizován také jako BaseLayer.
 * Není to sice úplně nezbytné, ale protože je to možné a ne příliš složité
 * nechť je to tak. Pak je jednoduší přizpůsobit to platformě.
 */
static Swdp      l4;

/// Tohle vyjmeme ze třídy - jen to by se mohlo měnit po přidání dalšího targetu.
void GdbServer::Scan (void) {
  if (target) { target->remove(); delete target; target = 0; }; // jedeme nanovo
//  if (probe (new STM32F4  (this, "STM32F4X")))      return;   // nechodi
  if (probe (new STM32F1  (this, "STM32FXX")))      return;
  if (probe (new LPC11XX  (this, "LPC11Xxx")))      return;
  if (probe (new CortexMx (this, "ARM Cortex-Mx"))) return;
}

//extern void cmprint (void);
int main (void) {
  //cmprint();
  debug ("STM size=%ld\n", sizeof (STM32F1));
  
  l4 += l3 += l2 += l1;
  l1.Init();

  for (;;) {
    l3.Polling ();
    if (!l1.Fini()) break;
  }
  // pro ARM celkem zbytecne
  l1.Fini ();
  l3.Fini ();
  l4.Fini ();
  return 0;
}