#ifndef SWDP_H
#define SWDP_H

#include "baselayer.h"
/** @file
 *  @brief Fyzický přístup na SWD port.
 */
/// GPIO pins used for SWD - CLK
#define SWCLK_BIT 18
/// GPIO pins used for SWD - DATA
#define SWDIO_BIT 19

#include "LPC11Uxx.h"

typedef enum {
  GPIO_Mode_IN   = 0x00, /*!< GPIO Input Mode              */
  GPIO_Mode_OUT  = 0x01, /*!< GPIO Output Mode             */
} GPIODir_TypeDef;

/// Enum pro PortNumber
typedef enum {
  GpioPortA = 0,
  GpioPortB,
  GpioPortC,
  GpioPortD,
  GpioPortF
} GpioPortNum;

/**
  * 
  * @class GpioClass
  * @brief Obecný GPIO pin.
  * 
  * Všechny metody jsou konstantní, protože nemění data uvnitř třídy.
  * Vlastně ani nemohou, protože data jsou konstantní.
*/
class GpioClass {
  public:
    /** Konstruktor
    @param port GpioPortA | GpioPortB | GpioPortC | GpioPortD | GpioPortF
    @param no   číslo pinu na portu
    @param type IN, OUT, AF, AN default OUT 
    */
    GpioClass (GpioPortNum const port, const uint32_t no, const GPIODir_TypeDef type = GPIO_Mode_OUT) :
      io(port), pos(1UL << no) {
      // Povol hodiny
      LPC_SYSCON->SYSAHBCLKCTRL |= (1<<6);
      // A nastav pin.
      setDir  (type);
    };
    /// Nastav pin @param b na tuto hodnotu
    const void set (const bool b) const {
      if (b) LPC_GPIO->SET[io] = pos;
      else   LPC_GPIO->CLR[io] = pos;
    };
    /// Nastav pin na log. H
    const void set (void) const {
      LPC_GPIO->SET[io] = pos;
    };
    /// Nastav pin na log. L
    const void clr (void) const {
      LPC_GPIO->CLR[io] = pos;
    };
    /// Změň hodnotu pinu
    const void change (void) const {
      LPC_GPIO->NOT[io] = pos;
    };
    /// Načti logickou hodnotu na pinu
    const bool get (void) const {
      if (LPC_GPIO->PIN[io] & pos) return true;
      else                         return false;
    };
    void setDir (GPIODir_TypeDef p) {
      if (p) LPC_GPIO->DIR[io] |=  pos;
      else   LPC_GPIO->DIR[io] &= ~pos;
    }
  private:
    /// Port.
    GpioPortNum const    io;
    /// A pozice pinu na něm
    const uint32_t       pos;
  
};


/// SWD status responses. SWD_ACK is good.
#define SWD_ACK 0b001
#define SWD_WAIT 0b010
#define SWD_FAULT 0b100
#define SWD_PARITY 0b1000
/// Příkazy v swdPacket
enum swdCommands {
  cmdInit         ,     //!< init
  cmdLowAccess    ,     //!< čtení / zápis na základní úrovni SWD
};
/// SWD Paket pro základní komunikaci
struct swdPacket {
  enum swdCommands    cmd : 8;  //!< Init / IO
  uint8_t             APnDP;    //!< Acess nebo Data Point
  uint8_t             RnW;      //!< Read / Write
  uint8_t             adr;      //!< Adresa
  uint32_t            val;      //!< DATA
}__attribute__((packed));
typedef struct swdPacket swdPacket_t;
/** @brief Fyzický přístup na SWD piny.
 * 
 * Zapouzdření do třídy a zdědění BaseLayer umožňuje jednoduše začlenit do řetězce.
 * Pravda je taková, že to mělo být uděláno trochu jinak, psal jsem to z druhé
 * strany, tak to vyšlo takto, ale ten systém výměny dat je ne příliš zdařilý.
 * Nicméně to funguje.
 */
class Swdp : public BaseLayer {

  public:
    /// Konstruktor
    Swdp ();
    /**
     * @brief Místo destruktoru.
     * Destruktory statických tříd ve firmware působí problémy.
     * Má význam pro ladění, ve firmware nic nedělá.
     **/
    void     Fini       (void);    
    /**
     * @brief Přetížíme metodu Up()
     * Zde v datech vrací, co se pomocí SWD načetlo.
     * @param data ukazatel na data
     * @param len a jejich délka
     * @return uint32_t počet vrácených byte
     **/
    uint32_t Up         (char* data, uint32_t len);
  protected:
    /**
     * @brief Základní inicializace
     * Má význam pro ladění, ve firmware nic nedělá.
     * @param  ...
     * @return uint32_t
     **/
    uint32_t Init               (void);
    /**
     * @brief Připojení na SWD.
     *
     * @param  ...
     * @return uint32_t Core ID nebo 0
     **/
    uint32_t swdptap_init       (void);
    /**
     * @brief Fyzický přístup na SWD
     *
     * @param APnDP Acess nebo Data point
     * @param RnW Read nebo Write
     * @param addr Adresa
     * @param value Data
     * @return uint8_t kód úspěšnosti operace viz shora SDW status responses
     **/
    uint8_t  swdptap_low_access (uint8_t APnDP, uint8_t RnW, uint8_t addr, uint32_t* value);
    
    void turnaround (bool dir);
    bool bit_in     (void);
    void bit_out    (bool val);
    int  init       (void);
    void reset      (void);
    uint32_t seq_in        (int ticks);
    bool     seq_in_parity (uint32_t *ret, int ticks);
    void     seq_out       (uint32_t MS,   int ticks);
    void     seq_out_parity(uint32_t MS,   int ticks);

  private:
    GpioClass   swdio, swclk;
    /// Paket pro obousměrnou výměnu dat.
    swdPacket   UxD;
    bool        olddir;
};

#endif // SWDP_H
