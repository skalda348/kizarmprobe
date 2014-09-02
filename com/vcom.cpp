#include "cdclass.h"
#include "mirror.h"
#include "usart1.h"

static CDClass cdc    (true, & iAssoc0);
static Mirror  top;
static Usart1  serial (9600);

int main (void) {
  
  top += cdc;
  top += serial;
  
  cdc.connect();
  
  for (;;);
  
  return 0;
}
