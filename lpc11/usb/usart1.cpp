#include "usart1.h"

#define IER_RBR  0x01
#define IER_THRE 0x02
#define IER_RLS  0x04

#define IIR_PEND 0x01
#define IIR_RLS  0x03
#define IIR_RDA  0x02
#define IIR_CTI  0x06
#define IIR_THRE 0x01

#define LSR_RDR  0x01
#define LSR_OE   0x02
#define LSR_PE   0x04
#define LSR_FE   0x08
#define LSR_BI   0x10
#define LSR_THRE 0x20
#define LSR_TEMT 0x40
#define LSR_RXFE 0x80

Usart1*     Usart1::inst = 0;
char        Usart1::rxb[8];

void Usart1::irq (void) {
  volatile uint8_t intId;
  char*    pbuf;
  uint32_t len, n, ofs = 0;

  intId = (LPC_USART->IIR >> 1) & 0x07; /* check bit 1~3, interrupt identification */

  switch (intId) {
    case  IIR_RLS:              /* Receive Line Status */
      /* Read LSR will clear the interrupt */
      if (LPC_USART->LSR) return;
      break;
    case IIR_CTI:               /* Character timeout indicator */
    case IIR_RDA:               /* Receive Data Available */
      pbuf = getRxb();
      len  = 0;
      /* 4 chars are available in FIFO */
      while (LPC_USART->LSR & LSR_RDR) {
        pbuf[len++] = LPC_USART->RBR;
      }
      while (len) {
        n = inst->Up (pbuf + ofs, len);
        if (!n) break;          // nesmi zustat v preruseni
        len -= n;
        ofs += n;
      }
      break;
    case  IIR_THRE:             /* THRE, transmit holding register empty */
      inst->Write();
      break;
    }
  return;
}
extern "C" void UART_IRQHandler (void)__attribute__((alias("_ZN6Usart13irqEv")));

void Usart1::Write (void) {
  char  txc;
  while (LPC_USART->LSR & LSR_THRE) {
    if (tx.Read(txc)) {
      LPC_USART->THR = txc;
    } else break;
  }
}
uint32_t Usart1::Down (char* data, uint32_t len) {
  uint32_t n;
  for (n=0; n<len; n++) {
    if (!tx.Write(data[n])) break;
  }
  if (n) Write();
  return n;
}

Usart1::Usart1 (uint32_t baud) : BaseLayer(), tx(64) {
  if (inst) return;
  inst = this;
  /* Enable UART clock */
  LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 12);
  LPC_SYSCON->UARTCLKDIV = 0x1;     /* divided by 1 */

  LPC_IOCON->PIO0_18 &= ~0x07;    /*  UART I/O config */
  LPC_IOCON->PIO0_18 |= 0x01;     /* UART RXD */
  LPC_IOCON->PIO0_19 &= ~0x07;
  LPC_IOCON->PIO0_19 |= 0x01;     /* UART TXD */
  Init (baud, 0x03);              /* 8 bits, no Parity, 1 Stop bit */
  // Zkusime snizit prioritu USARTu.
  NVIC_SetPriority (UART_IRQn, 3);
  /* enable IRQ */
  NVIC_EnableIRQ   (UART_IRQn);
}
void Usart1::Init (uint32_t baud, uint8_t lcr) {
  uint32_t Fdiv = ( (SystemCoreClock / LPC_SYSCON->UARTCLKDIV) / 16) / baud ; /*baud rate */
  LPC_USART->IER  = 0;
  LPC_USART->LCR  = lcr | 0x80; /* DLAB = 1 */
  LPC_USART->DLM  = Fdiv / 256;
  LPC_USART->DLL  = Fdiv % 256;
  LPC_USART->LCR  = lcr;        /* DLAB = 0 */
  // LPC_USART->HDEN = 1;       /* Half duplex on pin TX */
  LPC_USART->FCR  = 0x07;       /* Enable and reset TX and RX FIFO.
                                      Rx trigger level 4 chars*/
  LPC_USART->IER = IER_RBR | IER_THRE | IER_RLS; /* Enable UART1 interrupt */
}

// Callback from USB CDC
uint32_t Usart1::setLine (CDC_LINE_CODING* line_coding) {
  uint32_t baud = 9600;
  uint8_t  lcr  = 0x3;               /* 8 bits, no Parity, 1 Stop bit */

  if (line_coding->bCharFormat)
    lcr |= (1 << 2);                 /* Number of stop bits */

  if (line_coding->bParityType) {    /* Parity bit type */
    lcr |= (1 << 3);
    lcr |= ( ( (line_coding->bParityType - 1) & 0x3) << 4);
  }
  if (line_coding->bDataBits) {      /* Number of data bits */
    lcr |= ( (line_coding->bDataBits - 5) & 0x3);
  } else {
    lcr |= 0x3;
  }
  baud = line_coding->dwDTERate;
  Init (baud, lcr);
  return 1;
}
/**
 * Trik - v Linuxu pokud pustíme picocom, nastaví linky pro flow control.
 * Tím zároveň povolíme USART. Pokud bude jinak zakázán, nebude do toho
 * kecat a bude od něj pokoj.
 * */
uint32_t Usart1::lineState (uint16_t state) {
  if (state & 1) {
    // Povolit USART
    LPC_USART->IER = IER_RBR | IER_THRE | IER_RLS;
  }
  else {
    // zakazat USART
    LPC_USART->IER = 0;
  }
  return 1;
}
#ifdef SERIAL
// Set callback address to struct CDCIndividual iAssoc 0/1
extern "C" uint32_t iA1LineCode  (
           CDC_LINE_CODING* line_coding)__attribute__((alias("_ZN6Usart17setLineEP16_CDC_LINE_CODING")));
extern "C" uint32_t iA1LineState (uint16_t state)__attribute__((alias("_ZN6Usart19lineStateEt")));
#else
extern "C" uint32_t iA0LineCode  (
           CDC_LINE_CODING* line_coding)__attribute__((alias("_ZN6Usart17setLineEP16_CDC_LINE_CODING")));
extern "C" uint32_t iA0LineState (uint16_t state)__attribute__((alias("_ZN6Usart19lineStateEt")));
#endif
