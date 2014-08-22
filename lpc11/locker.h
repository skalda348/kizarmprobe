#ifndef LOCKER_H
#define LOCKER_H

/** @file
 *  @brief Zamykání a odemykání vlákna.
 **/

/**
 * @brief Zamykání a odemykání vlákna.
 * 
 * Tohle vypadá jako třída dost zbytečně. Ale pokud ladíme na PC pod OS je přístup poněkud odlišný.
 * Tam se používá mutex, takže konstruktor a destruktor je potřeba. Ve firmware stačí zakázat
 * nebo povolit globálně přerušení. Zase - různé hlavičky pro různý účel pak umožňují nepoužít
 * v kódu ifdef. Je to sice dvojí práce, ale přehlednější.
 * Stačí, aby veřejné metody byly stejné.
 **/
static const unsigned LockerChunk = 0x80;
static const unsigned LockerLimit = 0x8000;
class Locker {

  public:
    /// Konstruktor
    Locker () { cnt = 0; max = LockerChunk; };
    /// Místo destruktoru
    void        Fini    (void) {};
    /// Uzamčení
    void        lock    (void) {
      asm volatile ("cpsid i");
    };
    /// Odemčení
    void        unlock  (void) {
      asm volatile ("cpsie i");
    };
    void       Reset    (void) {
      max = LockerChunk;
    };
    /// prodlouzeni smycky
    bool       NotDone  (void) {
      if (cnt++ < max)       return true;
      cnt  = 0;
      // Saturate max
      if (max > LockerLimit) return false;
      max += LockerChunk;
      return false;
    };
  private:      // ve fw nic
    unsigned cnt;
    unsigned max;
};

#endif // LOCKER_H
