#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "socket.h"

Socket::Socket() : BaseLayer() {
  conFd  = 0;
  sockFd = 0;
  loop   = false;
}
/*
Socket::~Socket() {

}
*/
void* Socket::Receiver (void) {
  int  n;
  socklen_t clilen;
  
  loop   = true;
  clilen = sizeof (cliaddr);
  conFd  = accept (sockFd, (struct sockaddr *) &cliaddr, &clilen);
  debug ("Accept\n");
  while (loop) {
    n = recvfrom (conFd, buf, SOCKBUFLEN, 0, (struct sockaddr *) &cliaddr, &clilen);
    //FifoWrite (&gFifoRx, mesg, n);
    BaseLayer::Up (buf, n);
    if (n <= 0) {
      loop = 0;
      break;
    }
    buf[n] = 0;
    // debug ("Received: %s\n", buf);
  }
  debug ("Connection closed\n");
  close (conFd);
  return NULL;
}

static void* ReceiveHandler (void* t) {
  Socket * tsock = (Socket*) t;
  return tsock->Receiver();
}

void Socket::Init (void) {
  struct sockaddr_in servaddr;

  sockFd = socket (AF_INET, SOCK_STREAM, 0);

  memset (&servaddr, 0, sizeof (servaddr));
  servaddr.sin_family      = AF_INET;
  servaddr.sin_addr.s_addr = htonl (INADDR_ANY);
  servaddr.sin_port        = htons (3333);
  bind  (sockFd, (struct sockaddr *) &servaddr, sizeof (servaddr));
  debug ("Bind\n");

  listen (sockFd, 1024);
  debug ("Listen\n");
  // Nechceme vice vlakenm staci 1 pripojeni.
  pthread_create  (&rec, NULL, ReceiveHandler, this);
  debug ("Thread\n");
}
bool Socket::Fini (void) {
  usleep (100000);
  if (loop) return loop;
  pthread_cancel (rec);
  close (sockFd);
  return false;
}

uint32_t Socket::Down (char *data, uint32_t len) {
  if (!conFd) return 0;
  data[len] = 0;
  debug ("%s::%d:%s\n", __func__, len, data);
  return sendto (conFd, data, len, 0, (struct sockaddr *) &cliaddr, sizeof (cliaddr));
}

