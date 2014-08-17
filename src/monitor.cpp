#include <stdio.h>
#include <string.h>
#include "baselayer.h"
#include "monitor.h"
#include "target.h"
#include "gdbserver.h"

Monitor::Monitor (const char* name) : CommandSet (name),
  cHelp ("help", "Display help for monitor commands"),
  cScan ("scan", "Scan SW-DP for devices") {
  gdb  = 0;
  base = 0;
  
  addCmd (cHelp);
  addCmd (cScan);
  addSet (this);
}
void Monitor::setServer (GdbServer *g) {
  gdb = g;
  cHelp.setServer (g);
  cScan.setServer (g);
}

void Monitor::addSet (CommandSet *s) {
  CommandSet * cs, * ls;
  if (!base) base = s;
  else {
    for (ls = cs = base; cs; cs = cs->getNext()) ls = cs;
    * ls += * s;
  }
}


int Monitor::command (char *line) {
  debug ("%s: %s\n", __func__, line);

  int l, argc = 0;
  char * s;
  // Initial estimate for argc
  for (s = line; *s; s++)
    if ( (*s == ' ') || (*s == '\t')) argc++;

  l = sizeof (const char *) * argc;
  const char *argv [l];

  // Tokenize line to find argv
  for (argc = 0,   argv[  argc] = strtok (line, " \t");
       argv[argc]; argv[++argc] = strtok (NULL, " \t"));
  /*
  debug ("argv len=%d bytes\n", l);
  for (l=0; l<argc; l++) debug ("argv[%2d]=\"%s\"\n", l, argv[l]);
  */
  if (base) {
    CommandSet * cst;
    for (cst = base; cst; cst = cst->getNext())
      if (cst->Handler (argc, argv)) break;
      
  }
  return 0;
}
/////////////////////////////////////////////////////////////////////////////////
int Monitor::Handler (int argc, const char* argv[]) {
  int res = CommandSet::Handler (argc, argv);
  switch (res) {
    case 1:  print ();      break;
    case 2:  gdb->Scan  (); break;
    default: break;
  }
  return res;
}
