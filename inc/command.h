#ifndef COMMAND_H
#define COMMAND_H

class GdbServer;

class Command {
  public:
    Command (const char* c, const char* h);
    void          setServer  (GdbServer * s);
    Command   &   operator+= (Command   & c);
    bool          Handler    (int argc, const char* argv[]);
    int           reply      (const char* fmt, ...);
    Command   *   getNext    (void);
    
    const char*   getCmd     (void);
    const char*   getHlp     (void);
    int           getNo      (void);
    void          setNo      (int n);
    
    void          print      (void);
  private:
    int         number;
    Command   * next;
    GdbServer * gdb;
    const char* cmd;
    const char* hlp;
};

#endif // COMMAND_H
