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

class Locker {

  public:
    /// Konstruktor
    Locker () {};
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
  private:      // ve fw nic
};

#endif // LOCKER_H
