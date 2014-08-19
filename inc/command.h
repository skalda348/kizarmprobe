#ifndef COMMAND_H
#define COMMAND_H

/**
 * @file
 * @brief Zpracování příkazů Monitoru (gdb "monitor")
 */

class GdbServer;

/**
 * @brief Zpracování příkazů Monitoru (gdb "monitor")
 * Původní C-čkový přístup byl jiný, možná jednodušší.
 * Dost se zamotává tím, že to potřebuje přístup zpět na GdbServer.
 */
class Command {
  public:
    /**
     * @brief Konstruktor
     * Včetně základních nastavení.
     * @param c řetězec znamená příkaz
     * @param h řetězec obsahuje co vypíše help
     **/
    Command (const char* c, const char* h);
    /// nastaví obslužný server
    void          setServer  (GdbServer * s);
    /// zařadí tuto třídu na konec seznamu
    Command   &   operator+= (Command   & c);
    /// vlastní obslužná funkce příkazu
    bool          Handler    (int argc, const char* argv[]);
    /// odpověď na příkaz (to právě vyžaduje ten GdbServer)
    int           reply      (const char* fmt, ...);
    /// Všechny tyhle getry a setry jsou jen proto, že data jsou privátní.
    Command   *   getNext    (void);
    /// Všechny tyhle getry a setry jsou jen proto, že data jsou privátní.
    const char*   getCmd     (void);
    /// Všechny tyhle getry a setry jsou jen proto, že data jsou privátní.
    const char*   getHlp     (void);
    /// Všechny tyhle getry a setry jsou jen proto, že data jsou privátní.
    int           getNo      (void);
    /// Všechny tyhle getry a setry jsou jen proto, že data jsou privátní.
    void          setNo      (int n);
    /// Výpis helpu
    void          print      (void);
  private:
    int         number;         //!< příkazy jsou číslovány
    Command   * next;           //!< následující (spojový seznam)
    GdbServer * gdb;            //!< server
    const char* cmd;            //!< příkaz
    const char* hlp;            //!< a jeho help
};

#endif // COMMAND_H
