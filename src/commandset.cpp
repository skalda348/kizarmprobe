#include <stdio.h>
#include "baselayer.h"
#include "command.h"
#include "commandset.h"


CommandSet::CommandSet (const char * n) : name (n) {
  root = 0;
  next = 0;
  prev = 0;
}

int CommandSet::Handler (int argc, const char *argv[]) {
  int res = 0;
  if (!argc) return res;
  Command * cmd;
  for (cmd = getRoot(); cmd; cmd = cmd->getNext()) {
    if (cmd->Handler (argc, argv)) {
      res = cmd->getNo ();
      //cmd->reply ("match [%d] : %s==%s", res, cmd->getCmd(), argv[0]);
      return res;
    }
  }
  return res;

}
CommandSet &CommandSet::operator+= (CommandSet & c) {
  next   = & c;
  c.prev = this;
  return * this;
}

void CommandSet::remove (void) {
  //debug ("Remove: %s\n", getName());
  if (prev) prev->next = next;
  if (next) next->prev = prev;
}

void CommandSet::addCmd (Command & c) {
  Command * cmd, * last;
  int n = 1;
  if (!root) root = & c;
  else {
    // na konec
    for (last = cmd = root; cmd; cmd = cmd->getNext(), n += 1)  last = cmd;
    // pridame
    * last += c;
  }
  c.setNo (n);
}
CommandSet * CommandSet::getNext (void) {
  return next;
}
Command *    CommandSet::getRoot (void) {
  return root;
}
const char * CommandSet::getName (void) {
  return name;
}

void CommandSet::setName (const char* n) {
  name = n;
}

void CommandSet::print (void) {
  root->reply ("CommandSet: %s\n", name);
  root->print();
  if (next) next->print();
}
