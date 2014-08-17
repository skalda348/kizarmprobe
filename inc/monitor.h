#ifndef MONITOR_H
#define MONITOR_H

#include "command.h"
#include "commandset.h"
class GdbServer;

class Monitor : public CommandSet {

  public:
    Monitor        (const char * name);
    void setServer (GdbServer  * g);
    void addSet    (CommandSet * set);
    int  command   (char* data);
    int  Handler   (int argc, const char* argv[]);
    
    Command      cHelp, cScan;
  private:
    CommandSet * base;
    GdbServer  * gdb;
};

#endif // MONITOR_H
