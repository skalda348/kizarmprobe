#ifndef COMMANDSET_H
#define COMMANDSET_H

class Command;

class CommandSet {

  public:
    CommandSet (const char * n);
    void                addCmd          (Command    & c);
    CommandSet *        getNext         (void);
    CommandSet &        operator+=      (CommandSet & c);
    
    Command    *        getRoot         (void);
    const char *        getName         (void);
    void                setName         (const char* name);
    
    void print (void);
    
    virtual void        remove          (void);
    virtual int         Handler         (int argc, const char* argv[]);
  private:
    const char *        name;
    Command    *        root;
    CommandSet *        next;
    CommandSet *        prev;
};

#endif // COMMANDSET_H
