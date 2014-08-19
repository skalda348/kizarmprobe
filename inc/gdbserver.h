#ifndef GDBSERVER_H
#define GDBSERVER_H

#include "baselayer.h"
#include "monitor.h"
#include "locker.h"
/**
 * @file
 * @brief Vlastní obsluha gdb paketů.
 **/

class Target;
class Monitor;

/**
 * @brief Vlastní obsluha gdb paketů.
 * Nejpodstatnější část celého programu.
 * Vše se děje v přerušení od USB, jen pokud target rozběhneme, pak se dotazujeme,
 * zda ještě běží v main() metodou Polling(). To se zamyká třídou Locker.
 * Parsování paketů pomocí sscanf() je hodně podobné jako v black magic, tato
 * funkce (jakož i jiné knihovní funkce) byla trochu zjednodušena a přidána do
 * vlastní knihovny libprobe.
 * Ono je to s knihovnami sporné. Pokud použiju systémovou, pak může jakékoli
 * vylepšení a jiná změna způsobit fatální chybu. Po těchto negativních zkušenostech
 * je to vyřešeno takto. A docela to i šetří místo ve flash.
 **/
class GdbServer : public BaseLayer {

  public:
    /// Konstruktor
    GdbServer();
    /// pro kompatibilitu s PC pro ladění
    void Fini            (void);
    /// Scan - po příkazu "monitor scan"
    void Scan            (void);
    /// Zde se zjišťuje, zda target běží.
    void Polling         (void);
    
    /// Vlastní parser paketů je zde
    uint32_t Up          (char *data, uint32_t len);
    /// Odpověď zpět do gdb (formátovaná)
    void gdb_putpacket_f (const char *fmt, ...);
    /// Odpověď zpět do gdb (neformátovaná)
    void gdb_out         (const char *buf);
    //int  gdb_outf        (const char *fmt, ...);
  protected:
    /// obsluha speciálních paketů
    void handle_q_packet (char *packet, int len);
    /// obsluha speciálních paketů
    void handle_v_packet (char *packet, int len);
    /// obsluha speciálních paketů
    void handle_z_packet (char *packet, int len);
    /// odpověď na q_packet
    void handle_q_string_reply (const char *str, const char *param);
    /// další forma odpovědi
    void gdb_putpacket   (const char *packet, int size);
    /// další forma odpovědi
    void gdb_putpacketz  (const char *packet);
    /// zjištění stavu targetu
    bool target_check    (void);
    /// Probe for @param n new target. If failed, n is delete, else n used as target.
    bool probe           (Target* n);
  public:
    /// Target je zapouzdřen sem. Vytváří se dynamicky v metodě Scan().
    Target  * target;
    /// Monitor - zpracování uživatelských příkazů gdb "monitor ..."
    Monitor   mon;
  private:
    int  size;                  //!< délka paketu
    int  signal;                //!< výsledek zastavení targetu
    bool single_step;           //!< po instrukcích
    volatile bool active;       //!< target běží
    char last_activity;         //!< a jeho poslední aktivita
    Locker  lock;               //!< zamykání při Polling()
};

#endif // GDBSERVER_H
