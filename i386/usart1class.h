#ifndef USART1CLASS_H
#define USART1CLASS_H

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <termios.h>
#include "baselayer.h"

#define BUFLEN 1024
// Bottom
class Usart1Class : public BaseLayer {
  public:
    Usart1Class   (uint32_t baud, const char* name);
    uint32_t  Up  (char* data, uint32_t len);
    uint32_t  Down(char* data, uint32_t len);
   ~Usart1Class   ();
  protected:
           void  ReadLoop      (void);
    static void* Usart1Handler (void* p) {
      Usart1Class* inst = (Usart1Class*) p;
      inst->ReadLoop();
      return NULL;
    };
  public:
    bool         running;
  private:
    const char*  id;         //!< Identifikátor třídy pro ladění
    char         rxbuf[BUFLEN];
    int          fd;
    pthread_t    rc;
};
#endif // USART1CLASS_H
