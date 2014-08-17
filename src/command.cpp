#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "command.h"
#include "gdbserver.h"

Command::Command (const char* c, const char* h) :
    cmd (c), hlp (h) {
  gdb    = 0;
  next   = 0;
  number = 0;
}
void Command::setServer (GdbServer *s) {
  gdb = s;
}

Command& Command::operator+= (Command & c) {
  next = & c;
  return * this;
}

bool Command::Handler (int argc, const char* argv[]) {
  if (!argc) return false;
  int n = strlen (argv[0]);
  if (strncmp (argv[0], cmd, n)) return false;
  return true;
}

Command* Command::getNext (void) {
  return next;
}

int Command::reply (const char* fmt, ...) {
  va_list ap;
  char* buf;

  va_start (ap, fmt);
  int i = vasprintf (&buf, fmt, ap);
  if (gdb) gdb->gdb_out (buf);
  free (buf);
  va_end (ap);
  return i;

}
const char *Command::getCmd (void) {
  return cmd;
}
const char *Command::getHlp (void) {
  return hlp;
}
int Command::getNo (void) {
  return number;
}
void Command::setNo (int n) {
  number = n;
}
void Command::print (void) {
  reply ("\t[%d]: %s -- %s\n", number, cmd, hlp);
  if (next) next->print();
}

