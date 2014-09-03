#ifndef SOCKET_H
#define SOCKET_H

#include <pthread.h>
#include <netinet/in.h>
#include "baselayer.h"

#define SOCKBUFLEN 64



class CDClass : public BaseLayer {

  public:
    CDClass (const int port);
    // ~CDClass();
    void  Init     (void);
    bool  Fini     (void);
    void* Receiver (void);
    uint32_t Down  (char *data, uint32_t len);
  private:
    volatile  bool    loop;
    int       sockFd, conFd;
    pthread_t rec;
    struct    sockaddr_in cliaddr;
    char      buf [SOCKBUFLEN];
};

#endif // SOCKET_H
