#ifndef LOCKER_H
#define LOCKER_H

class Locker {

  public:
    Locker () {};
    void        Fini    (void) {};
    void        lock    (void) {
      asm volatile ("cpsid i");
    };
    void        unlock  (void) {
      asm volatile ("cpsie i");
    };
  private:
};

#endif // LOCKER_H
