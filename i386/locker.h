#ifndef LOCKER_H
#define LOCKER_H
#include <pthread.h>

class Locker {

  public:
    Locker () {
      pthread_mutex_init (& mutex, NULL);
    }
    void        Fini    (void) {
      pthread_mutex_destroy (& mutex);
    }
    void        lock    (void) {
      pthread_mutex_lock (& mutex);
    }
    void        unlock  (void) {
      pthread_mutex_unlock (& mutex);
    }
    void       Reset    (void) {
    };
    bool       NotDone  (void) {
      return false;
    };
  private:
    pthread_mutex_t mutex;
};

#endif // LOCKER_H
