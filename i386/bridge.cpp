#include <signal.h>
#include "mirror.h"
#include "cdclass.h"
#include "usart1class.h"

static Mirror      top;
static CDClass     cdc    (0);
static Usart1Class serial (57600, "/dev/ttyACM0");

static void signothing (int sig) {
  printf ("Ctrl-C received\n");
  serial.running = false;
}

int main (void) {
  signal (SIGINT, signothing);
  cdc.Init();
  top += cdc;
  top += serial;
  while (serial.running) {
    usleep(10000);
  }
  cdc.Fini();
  printf ("Konec\n");
  return 0;
}
