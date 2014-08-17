#ifndef BASELAYER_H
#define BASELAYER_H
#include <stdlib.h>
#include <stdint.h>

// Vnitřní makra gcc, g++
#if __ARM_EABI__ && __ARM_ARCH_6M__
  #define ARCH_CM0 1
#endif //__ARM_EABI__ ...

#if ARCH_CM0
  #define debug(...)
#else  // ARCH_CM0
  #ifdef DEBUG
    #define debug printf
  #else  // DEBUG
    #define debug(...)
  #endif // DEBUG
#endif // ARCH_CM0
/** @file
 * @brief Bázová třída pro stack trochu obecnějšího komunikačního protokolu.
 * 
 * @class BaseLayer
 * @brief Od této třídy budeme dále odvozovat ostatní.
 * 
 * Použití virtuálních metod umožňuje polymorfizmus. Pokud v odvozené třídě přetížíme nějakou
 * virtuální metodu, bude se používat ta přetížená, polud ne, použije se ta virtuální.
 * Třída nemá *.cpp, všechny metody jsou jednoduché a tedy inline.
*/
//! [BaseLayer example]
class BaseLayer {
  public:
    /** Konstruktor
    */
    BaseLayer () {
      pUp   = NULL;
      pDown = NULL;
    };
    /** Virtuální metoda, přesouvající data směrem nahoru, pokud s nimi nechceme dělat něco jiného.
    @param data ukazatel na pole dat
    @param len  delka dat v bytech
    @return počet přenesených bytů
    */
    virtual uint32_t    Up   (char* data, uint32_t len) {
      if (pUp) return pUp->Up (data, len);
      return 0;
    };
    /** Virtuální metoda, přesouvající data směrem dolů, pokud s nimi nechceme dělat něco jiného.
    @param data ukazatel na pole dat
    @param len  delka dat v bytech
    @return počet přenesených bytů
    */
    virtual uint32_t    Down (char* data, uint32_t len) {
      if (pDown) return pDown->Down (data, len); 
      return len;
    };
    /** Zřetězení stacku
    @param bl Třída, ležící pod, spodní
    @return Odkaz na tuto třídu (aby se to dalo řetězit)
    */
    virtual BaseLayer&  operator += (BaseLayer& bl) {
      bl.setUp (this);  // ta spodní bude volat při Up tuto třídu
      setDown  (&bl );  // a tato třída bude volat při Down tu spodní
      return *this;
    };
    /** Getter pro pDown
    @return pDown
    */
    BaseLayer* getDown (void) const { return pDown; };
  protected:
    /** Lokální setter pro pUp
    @param p Co budeme do pUp dávat
    */
    void setUp   (BaseLayer* p) { pUp   = p; };
    /** Lokální setter pro pDown
    @param p Co budeme do pDown dávat
    */
    void setDown (BaseLayer* p) { pDown = p; };
  private:
    // Ono to je vlastně oboustranně vázaný spojový seznam.
    BaseLayer*  pUp;        //!< Ukazatel na třídu, která bude dále volat Up
    BaseLayer*  pDown;      //!< Ukazatel na třídu, která bude dále volat Down
};
//! [BaseLayer example]

#endif // BASELAYER_H
