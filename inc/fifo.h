#ifndef FIFO_H
#define FIFO_H
//#include "core_cm0.h"

extern volatile int gblMutex;

/// V tomto konkrétním případě není lock / unlock potřeba.
static inline void fifo_lock (void) {
  asm volatile ("cpsid i");
  gblMutex++;
}
/// V tomto konkrétním případě není lock / unlock potřeba.
static inline void fifo_unlock (void) {
  if (--gblMutex) return;
  asm volatile ("cpsie i");
}

//#define FIFODEPTH (1<<10)
//#define FIFOMASK  (FIFODEPTH-1)
/** @file
 * @brief Fifo čili fronta.
 * 
 * @class Fifo
 * @brief Fifo čili fronta.
 * 
 * Fifo použité v obsluze USB zkusíme napsat jako šablonu. Fakt je, že v tomto případě kód neroste.
 * Zřejmě je překladač docela inteligentní. A jde tak ukládat do fifo různá data, včetně složitých objektů.
 * Fifo je tak jednoduché, že může mít všechny metody v hlavičce, tedy default inline.
 * Je však třeba zajistit atomičnost inkrementace a dekrementace délky dat. Použita je metoda se zákazem
 * přerušení a snad funguje. Ona totiž pravděpodobnost, že se proces v přerušení zrovna trefí do té
 * in(de)krementace je tak malá, že je problém to otestovat na selhání, takže to celkem spolehlivě funguje
 * i bez ošetření té bezpečnosti. A u některých architektur je inkrementace či dekrementace buňky v paměti
 * atomická v principu. U této však nikoli. Viz :
 * 
  @code
      ...    safeDec();  ...
    8000364:       b672            cpsid   i
    8000366:       6d44            ldr     r4, [r0, #84]   ; 0x54
    8000368:       3c01            subs    r4, #1
    800036a:       6544            str     r4, [r0, #84]   ; 0x54
    8000328:       b662            cpsie   i
  @endcode
  
  Zde to nakonec není potřeba - do fronty se zapisuje a čte v přerušení a jediný případ, kdy se zapisuje
  v main() je stejně ošetřen uzamčením, aby se zapsal do fifo celý paket naráz.
  
  Nakonec bylo lepší udělat pořádný mutex.
*/
//! [Fifo class example]
template <class T> class Fifo {
  public:   // veřejné metody
    /// Parametr konstruktoru by měla být hloubka FIFO, ale pak by musela být dynamická alokace paměti.
    Fifo (const unsigned depth) : max (depth) {
      buf = new T [depth];
      rdi = 0; wri = 0; len = 0;
    };
    /// Zápis do FIFO
    /// @param  c odkaz na jeden prvek co bude zapsán
    /// @return true, pokud není plný
    bool Write (const T& c) {
      if (len < max) { buf [wri++] = c; saturate (wri); safeInc(); return true; }
      else return false;
    };
    /// Čtení z FIFO
    /// @param  c odkaz na jeden prvek, kam to bude načteno
    /// @return true, pokud není prázdný
    bool Read  (T& c) {
      if (len > 0) { c = buf [rdi++];   saturate (rdi); safeDec(); return true; }
      else return false;
    };
  protected:  // chráněné metody
    /// Saturace indexu - ochrana před zápisem/čtením mimo pole dat
    /// @param index odkaz na saturovaný index
    void saturate (volatile int& index) {
      //index &= FIFOMASK;  // FIFODEPTH je rovno 2 ** n, kde n je celé číslo. 
      if (index < max) return; index = 0; // FIFODEPTH obecně int
    };
    /// Atomická inkrementace délky dat
    void safeInc (void) { fifo_lock(); len++; fifo_unlock(); };
    /// Atomická dekrementace délky dat
    void safeDec (void) { fifo_lock(); len--; fifo_unlock(); };
  private:    // privátní data
    const int    max;
    T     *      buf;             //!< buffer na data
    volatile int rdi, wri, len;   //!< pomocné proměnné
};
//! [Fifo class example]

#endif // FIFO_H
