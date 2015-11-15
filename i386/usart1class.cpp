#include <termios.h>

#include "usart1class.h"


Usart1Class::Usart1Class (uint32_t baud, const char* name) : BaseLayer() {
  id = name;
  running = false;
  fd = ::open (id, O_RDWR);
  if (fd < 0) return;
  
  struct termios LineFlags;
  int attr = tcgetattr (fd, &LineFlags);
  if (attr) {
    printf ("Nelze zjistit parametry portu %s\r\n", name);
    ::close  (fd);
    return;
  }
  // nastaveni komunikacnich parametru do struct termios
  LineFlags.c_iflag = IGNPAR;           // ignoruj chyby parity
  LineFlags.c_oflag = 0;                // normalni nastaveni
  LineFlags.c_cflag = CS8 | CREAD | CLOCAL; // 8-bit, povol prijem
  LineFlags.c_lflag = 0;                // Raw input bez echa
  LineFlags.c_cc [VMIN]  = 1;           // minimalni pocet znaku pro cteni
  LineFlags.c_cc [VTIME] = 0;           // timer nepouzit
  
  cfsetospeed (&LineFlags, baud);
  cfsetispeed (&LineFlags, baud);
  tcsetattr   (fd, TCSANOW, &LineFlags);
  fcntl       (fd, F_SETFL, 0);
  
  printf ("Port %s opened\r\n", id);
  usleep (10000);
  running = true;
  pthread_create (&rc, NULL, Usart1Class::Usart1Handler,  this);
}

Usart1Class::~Usart1Class() {
  if (!running) return;
  running = false;
  pthread_cancel (rc);
  ::close (fd);
  printf ("Port %s closed\r\n", id);
}

uint32_t Usart1Class::Down (char* data, uint32_t len) {
  if (!running) return 0;
  data [len] = '\0';
  printf ("\n%s.Down %3d:\"%s\"\r\n", id, len, data);
  return (uint32_t) ::write (fd, data, len);
}

void Usart1Class::ReadLoop (void) {
  int n; char c;
  while (running) {
    // Nutno číst po jednom, jinak to po prvním čtení zdechne.
    n = ::read (fd, rxbuf, 1);
    if (!n) continue;
    c = rxbuf[0];
    putchar (c);
    // debug ("%s: read %d \"%s\"\r\n", id, n, rxbuf);
    BaseLayer::Up (rxbuf, n);
  }
}
