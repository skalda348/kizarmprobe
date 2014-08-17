#include "swdp.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>

#undef  debug
#define debug(...)

Swdp::Swdp () : BaseLayer() {
  const char* name = "/dev/ttyACM0";
  dev = open (name, O_RDWR);
  printf ("DEV OPEN %s=%d\n", name, dev);
  if (dev < 0) {
    printf ("Nelze otevrit %s\n", name);
    return;
  }

}
void Swdp::Fini (void) {
  printf ("DEV CLOSE %d\n", dev);
  close (dev);
}
uint32_t Swdp::Up (char *data, uint32_t len) {
  if (!len) {
    return Init ();
  }
  memcpy (&TxD, data, len);
  Write();
  Read ();
  memcpy (data, &RxD, len);
  return len;
}
uint32_t Swdp::Init (void) {
  TxD.cmd = cmdInit;
  Write();
  Read ();
  return RxD.val;
}
void Swdp::Read (void) {
  unsigned int rdn = 0;
  //unsigned int len = 64;
  unsigned int cst;
  char* ptr = (char*) & RxD;
  
  debug ("Enter to read, hlen=%ld\n", sizeof (RxD));
  for (;;) {
    cst = read (dev, ptr + rdn, 1);
    //printf ("readen %d, hlen=%d\n", cst, sizeof (RxD));
    rdn += cst;
    if (rdn == sizeof (RxD)) break;
  }
  debug ("Swdp::Read : %d\n", rdn);
}
void Swdp::Write (void) {
  unsigned int len = sizeof (TxD);
  unsigned int cst, wtn, ofs = 0;
  char* ptr = (char*) & TxD;
  
  while (len) {
    if (len > 64) wtn = 64;
    else          wtn = len;
    cst = write (dev, ptr + ofs, wtn);
    //printf ("writen %d (%s)\n", cst, strerror (errno));
    ofs += cst;
    len -= wtn;
  }
  debug ("Swdp::Write : %d\n", ofs);
}

