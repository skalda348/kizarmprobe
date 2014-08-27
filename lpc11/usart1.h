#ifndef USART1_H
#define USART1_H

#include "cdclass.h"
#include "fifo.h"

class Usart1 : public BaseLayer {
  public:
    Usart1              (uint32_t baud);
    void         Write  (void);
    uint32_t     Down   (char* data, uint32_t len);
    
    static ErrorCode_t  setLine (USBD_HANDLE_T hCDC, CDC_LINE_CODING* line_coding);
    static char*        getRxb  (void) {
      return rxb;
    };
    static void  irq    (void);
  protected:
    static void  Init   (uint32_t baud, uint8_t lcr);
  public:
    static Usart1  *   inst;
  private:
    static char        rxb[8];
    Fifo  <char>       tx;
};

#endif // USART1_H
