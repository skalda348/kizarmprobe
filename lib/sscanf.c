#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
//#include "hack.h"

/**
 * Tato sscanf verze je pouze pro blackmagic probe - umí jen to co je potřeba.
 * A toho není mnoho (fmt c,d,n,x,X). Zato je to krátké. 
 * 
 * strtol a strtoul převzaty z newlib
 */


#define Isspace(c)	((c == ' ') || (c == '\t') || (c=='\n') || (c=='\v') \
                         || (c == '\r') || (c == '\f'))


#define DEBUG(...)

#define LONG_MAX __LONG_MAX__
#define LONG_MIN (-(LONG_MAX-1))
#define ULONG_MAX 0xFFFFFFFFUL
/*
typedef __builtin_va_list va_list;

#define va_start(v,l) __builtin_va_start(v,l)
#define va_end(v)     __builtin_va_end(v)
#define va_arg(v,l)   __builtin_va_arg(v,l)
#define va_copy(d,s)  __builtin_va_copy(d,s)
*/
#define BUF 40
#define LONG     0x01   /* l: long or double */

enum eFormatState {
  LiteralState,
  FormatState,
};

int vsscanf (const char* str, const char* fmt, va_list ap) {
  register char   c;     /* character from format, or conversion */
  register int    width; /* field width, or 0 */
  register long   n;          /* handy integer */
  register int    flags;      /* flags as defined above */
  char           *p0;         /* saves original value of p when necessary */
  int             nassigned;  /* number of fields assigned */
  int             nread;      /* number of characters consumed from fp */
  int             base = 0;   /* base argument to strtol/strtoul */

  unsigned long (*ccfn) (const char*, char**, int) = 0;      /* conversion function (strtol/strtoul) */
  //char buf[BUF];              /* buffer for numeric conversions */
  enum eFormatState state = LiteralState;

  int   *ip;
  long  *lp;

  nassigned = 0;
  nread     = 0;
  flags     = 0;
  width     = 0;
  for (;;) {
    c = *fmt++;
    DEBUG ("Read %c\n", c);
    if (c == '\0') return nassigned;
    switch (state) {
    case LiteralState:
      DEBUG ("Literal State\n");
      switch (c) {
      case '%':
        state = FormatState;
        break;
      default:
        if (c != *str++) return nassigned;
        else nread++;
        break;
      }
      break;
    case FormatState:
      DEBUG ("Format  State\n");
      switch (c) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        width = 10 * width + c - '0';
        break;
      case 'l':
      case 'L':
        flags |= LONG;
        break;
      case 'n':
        if (flags & LONG) {
          lp = va_arg (ap, long *);
          *lp = nread;
        } else {
          ip = va_arg (ap, int *);
          *ip = nread;
        }
        state = LiteralState;
        continue;
      case 'd':
        base = 10;
        ccfn = (unsigned long (*) ()) strtol;
        break;
      case 'x':
      case 'X':
        base = 16;
        ccfn = strtoul;
        break;
      case 'c':
        p0 = va_arg (ap, char *);
        *p0 = *str++;
        nassigned += 1;
        nread     += 1;
        break;
      }
      switch (c) {
      case 'd':
      case 'x':
      case 'X':
        DEBUG ("call convert\n");
        if (flags & LONG) {
          lp = va_arg (ap, long *);
          *lp = ccfn (str, &p0, base);
        } else {
          ip = va_arg (ap, int *);
          *ip = ccfn (str, &p0, base);
        }
        if (str != p0) {
          n = p0 - str;
          DEBUG ("%ld converted, width=%d\n", n, width);
          str   += n;
          nread += n;
          nassigned += 1;
        }
        width = 0;
        flags = 0;
        state = LiteralState;
        break;
      }
      break;
    default:
      DEBUG ("Chyba stavu\n");
      break;
    }
  }
  return nassigned;
}

int sscanf (const char* str, const char* fmt, ...) {
  va_list ap;
  int result;
  va_start (ap, fmt);
  result = vsscanf (str, fmt, ap);
  va_end (ap);
  return result;
}

long strtol (const char* s, char** ptr, int base) {
  int minus = 0;
  unsigned long tmp;
  const char   *start = s;
  char *eptr;

  if (s == NULL) {
    if (!ptr)
      *ptr = (char *) start;
    return 0L;
  }
  while (Isspace (*s)) s++;
  if (*s == '-') {
    s++;
    minus = 1;
  } else if (*s == '+')  s++;

  tmp = strtoul (s, &eptr, base);
  if (ptr != NULL)
    *ptr = (char *) ( (eptr == s) ? start : eptr);
  if (tmp > (minus ? - (unsigned long) LONG_MIN : (unsigned long) LONG_MAX)) {
    return (minus ? LONG_MIN : LONG_MAX);
  }
  return (minus ? (long) - tmp : (long) tmp);
}
unsigned long strtoul (const char* s, char** ptr, int base) {
  unsigned long total = 0;
  unsigned digit;
  int radix;
  const char *start  = s;
  int did_conversion = 0;
  int overflow = 0;
  int minus = 0;
  unsigned long maxdiv, maxrem;

  if (s == NULL) {
    if (!ptr)
      *ptr = (char *) start;
    return 0L;
  }

  while (Isspace (*s))
    s++;

  if (*s == '-') {
    s++;
    minus = 1;
  } else if (*s == '+')
    s++;

  radix = base;
  if (base == 0 || base == 16) {
    /*
     * try to infer radix from the string (assume decimal).
     * accept leading 0x or 0X for base 16.
     */
    if (*s == '0') {
      did_conversion = 1;
      if (base == 0)
        radix = 8;    /* guess it's octal */
      s++;      /* (but check for hex) */
      if (*s == 'X' || *s == 'x') {
        did_conversion = 0;
        s++;
        radix = 16;
      }
    }
  }
  if (radix == 0)
    radix = 10;

  maxdiv = ULONG_MAX / radix;
  maxrem = ULONG_MAX % radix;

  while ( (digit = *s) != 0) {
    if (digit >= '0' && digit <= '9' && digit < ('0' + radix))
      digit -= '0';
    else if (radix > 10) {
      if (digit >= 'a' && digit < ('a' + radix - 10))
        digit = digit - 'a' + 10;
      else if (digit >= 'A' && digit < ('A' + radix - 10))
        digit = digit - 'A' + 10;
      else
        break;
    } else
      break;
    did_conversion = 1;
    if (total > maxdiv
        || (total == maxdiv && digit > maxrem))
      overflow = 1;
    total = (total * radix) + digit;
    s++;
  }
  if (overflow) {
    if (ptr != NULL)
      *ptr = (char *) s;
    return ULONG_MAX;
  }
  if (ptr != NULL)
    *ptr = (char *) ( (did_conversion) ? (char *) s : start);
  return minus ? - total : total;
}
