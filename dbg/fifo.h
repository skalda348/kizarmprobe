#ifndef FIFO_H
#define FIFO_H

static inline void fifo_lock (void) {
  asm volatile ("cpsid i");
}
static inline void fifo_unlock (void) {
  asm volatile ("cpsie i");
}

#define FIFODEPTH (1<<8)
#define FIFOMASK  (FIFODEPTH-1)
/** @file
 * @brief Fifo čili fronta.
 * 
 * @class Fifo
 * @brief Fifo čili fronta.
 * 
 *  Tato "kopie" je debug verzí - není pak potřeba ty ifdef atd.
 * Je trochu zkrácená na 256 bytů (stačilo by i méně) a zamyká inkrementace.
 * Jinak je to stejné.
*/
//! [Fifo class example]
template <class T> class Fifo {
  public:   // veřejné metody
    /// Parametr konstruktoru by měla být hloubka FIFO, ale pak by musela být dynamická alokace paměti.
    Fifo (const bool b, const unsigned x) { rdi = 0; wri = 0; len = 0; };
    /// Zápis do FIFO
    /// @param  c odkaz na jeden prvek co bude zapsán
    /// @return true, pokud není plný
    bool Write (const T& c) {
      if (len < FIFODEPTH) { buf [wri++] = c; saturate (wri); safeInc(); return true; }
      else return false;
    };
    /// Čtení z FIFO
    /// @param  c odkaz na jeden prvek, kam to bude načteno
    /// @return true, pokud není prázdný
    bool Read  (T& c) {
      if (len > 0) {     c = buf [rdi++];     saturate (rdi); safeDec(); return true; }
      else return false;
    };
  protected:  // chráněné metody
    /// Saturace indexu - ochrana před zápisem/čtením mimo pole dat
    /// @param index odkaz na saturovaný index
    void saturate (volatile int& index) {
      index &= FIFOMASK;  // FIFODEPTH je rovno 2 ** n, kde n je celé číslo. 
      // if (index < FIFODEPTH) return; index = 0; // FIFODEPTH obecně int, nezkoušeno
    };
    /// Atomická inkrementace délky dat
    void safeInc (void) { fifo_lock(); len++; fifo_unlock(); };
    /// Atomická dekrementace délky dat
    void safeDec (void) { fifo_lock(); len--; fifo_unlock(); };
  private:    // privátní data
    T            buf[FIFODEPTH];  //!< buffer na data
    volatile int rdi, wri, len;   //!< pomocné proměnné
};
//! [Fifo class example]

#endif // FIFO_H
