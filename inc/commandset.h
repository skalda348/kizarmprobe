#ifndef COMMANDSET_H
#define COMMANDSET_H



/**
 * @file
 * @brief Zapouzdření skupiny příkazů
 **/
class Command;

/**
 * @brief Zapouzdření skupiny příkazů
 * I původní řešení mělo něco podobného.
 * Zde je to oboustranně vázaný spojový seznam, aby to šlo případně vyhodit.
 **/
class CommandSet {

  public:
    /**
     * @brief Konstruktor
     * @param n Každá skupina má svúj název
     **/
    CommandSet (const char * n);
    /// Přidá příkaz na konec
    void                addCmd          (Command    & c);
    /// getter pro privátní data
    CommandSet *        getNext         (void);
    /// Zařadí skupinu příkazů do seznamu
    CommandSet &        operator+=      (CommandSet & c);
    /// getter pro privátní data
    Command    *        getRoot         (void);
    /// getter pro privátní data
    const char *        getName         (void);
    /// setter pro privátní data
    void                setName         (const char* name);
    /// Help
    void print (void);
    /// Vyhodí toto ze seznamu
    virtual void        remove          (void);
    /// Společná obsluha - @return číslo příkazu při shodě, jinak 0
    virtual int         Handler         (int argc, const char* argv[]);
  private:
    const char *        name;   //!< Jméno tt. sady
    Command    *        root;   //!< první příkaz
    CommandSet *        next;   //!< další sada
    CommandSet *        prev;   //!< předchozí sada
};

#endif // COMMANDSET_H
