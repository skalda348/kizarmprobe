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
/// A příslušné bitové masky
#define SWCLK_BIT_MSK (1<<SWCLK_BIT)
/// A příslušné bitové masky
#define SWDIO_BIT_MSK (1<<SWDIO_BIT)

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
  private:
    /// Paket pro obousměrnou výměnu dat.
    swdPacket   UxD;
};

#endif // SWDP_H
