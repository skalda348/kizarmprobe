/* $Id: malloc.c,v 1.5 2004/04/29 15:41:47 pefo Exp $ */

/*
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
*/
/*
 * Pokud chceme použít knihovní malloc-free, definovat v makefile -DUSE_NANO_MALLOC.
 * Zde použitý algoritmus podle http://mirror.fsf.org/pmon2000/pmon/src/lib/libc/malloc.c
 * není příliš otestován. A jako všechny ostatní algoritmy pro správu haldy není příliš
 * průhledný. Je však relativně jednoduchý. Pro CM0 musí být začátek _HEAP_START zarovnán na
 * celé slovo (align 4), jinak to nechodí. To není zcela samozřejmé a na jiných architekturách
 * to chodí (bohužel) i nezarovnané. Zde fixováno vytvořením sekce .heap, ALIGN 4.
 * */


#define HEAPLEN 0x0C00

#ifdef __thumb__
  #define HEAPSECT __attribute__((section(".heap"),aligned(4)))
#define printf(...)
#else // ladeni na PC
  #define HEAPSECT
  #include <stdio.h>
#endif //__thumb__

static char  HEAPSECT  _HEAP_START [HEAPLEN];
static const char*     _HEAP_MAX = _HEAP_START + HEAPLEN;

#ifndef USE_NANO_MALLOC

#define NULL   ((void *)0)

typedef unsigned long ALIGN;

union header {
  struct {
    union header   *ptr;
    unsigned long   size;
  } s;
  ALIGN             x;
};

typedef union header HEADER;

static HEADER   base;
static HEADER  *allocp;

#define NULL   ((void *)0)
#define NALLOC 64


static HEADER *morecore (unsigned long);

void *malloc (unsigned long nbytes) {
  HEADER *p, *q;                // K&R called q, prevp
  unsigned nunits;
  
  nunits = (nbytes + sizeof (HEADER) - 1) / sizeof (HEADER) + 1;
  //printf ("malloc %ld bytes, nunits=%d\n", nbytes, nunits);
  
  if ((q = allocp) == NULL) {   // no free list yet 
    base.s.ptr = allocp = q = &base;
    base.s.size = 0;
  }
  for (p = q->s.ptr;; q = p, p = p->s.ptr) {
    if (p->s.size >= nunits) {  // big enough 
      if (p->s.size == nunits)  // exactly 
        q->s.ptr = p->s.ptr;
      else {                    // allocate tail end 
        p->s.size -= nunits;
        p += p->s.size;
        p->s.size  = nunits;
      }
      allocp = q;
      //printf ("malloc return ptr %p\n", (char*) (p + 1));
      return ((char *) (p + 1));
    }
    if (p == allocp)
      if ((p = morecore (nunits)) == NULL)  return NULL;
  }
}


int allocsize (void *ap);

void free (void *ap) {
  HEADER *p, *q;

  //printf ("free %p %d\n", ap, allocsize (ap));
  p = (HEADER *) ap - 1;
  for (q = allocp; ! (p > q && p < q->s.ptr); q = q->s.ptr)
    if (q >= q->s.ptr && (p > q || p < q->s.ptr))
      break;

  if (p + p->s.size == q->s.ptr) {
    p->s.size += q->s.ptr->s.size;
    p->s.ptr = q->s.ptr->s.ptr;
  } else
    p->s.ptr = q->s.ptr;
  if (q + q->s.size == p) {
    q->s.size += p->s.size;
    q->s.ptr = p->s.ptr;
  } else
    q->s.ptr = p;
  allocp = q;
}

static void *krSbrk (unsigned long size) {
  static const char * heap_ptr;
  const char *        old_heap_ptr;
  static unsigned int init_sbrk = 0;
  //printf ("krSbrk %ld\n", size);
  /* heap_ptr is initialized to HEAP_START */
  if (init_sbrk == 0) {
    heap_ptr = _HEAP_START;
    init_sbrk = 1;
  }
  old_heap_ptr = heap_ptr;
  if ((heap_ptr + size) > _HEAP_MAX) {
    return (void *) -1;
  }
  heap_ptr += size;
  return (void *)old_heap_ptr;
}

static HEADER  *morecore (unsigned long nu) {
  char   *cp;
  HEADER *up;
  int     rnu;

  rnu = NALLOC * ((nu + NALLOC - 1) / NALLOC);
  //printf("morecore %ld units, %d rnu\n", nu, rnu);
  cp  = krSbrk (rnu * sizeof (HEADER));
  if (cp == NULL) return NULL;
  up = (HEADER *) cp;
  up->s.size = rnu;
  free ((char *) (up + 1));
  return (allocp);
}

int allocsize (void *ap) {
  HEADER *p;

  p = (HEADER *) ap - 1;
  return (p->s.size * sizeof (HEADER));
}

void* calloc (unsigned long nmemb, unsigned long size) {
  unsigned long i, req = nmemb * size;
  if (!req) req++;
  void* res = malloc(req);
  char* clr = (char*) res;
  for (i=0; i<req; i++) clr[i] = 0;
  return res;
}
/// pouze zkraceni, pri prodlozeni to muze cist nekde kde nema
void *realloc(void *ptr, unsigned long size) {
  unsigned char* nptr = malloc (size);
  unsigned char* optr = (unsigned char*) ptr;
  int i;
  for (i=0; i<size; i++) nptr[i] = optr[i];
  free (ptr);
  return (void*) nptr;
}
#else // USE_NANO_MALLOC

#ifdef __thumb__
void * _sbrk (unsigned long size) {
  //uprintf("sbrk %d\r\n", size);
  
  static const char * heap_ptr;
  const char *        old_heap_ptr;
  static unsigned int init_sbrk = 0;
  if (init_sbrk == 0) {
    heap_ptr = _HEAP_START;
    init_sbrk = 1;
  }
  old_heap_ptr = heap_ptr;
  if ((heap_ptr + size) > _HEAP_MAX) {
    return (void *) -1;
  }
  heap_ptr += size;
  return (void *)old_heap_ptr;
}
#endif // __thumb__

#include <stdlib.h>
void* calloc (unsigned long nmemb, unsigned long size) {
  unsigned long i, req = nmemb * size;
  if (!req) req++;
  void* res = malloc(req);
  char* clr = (char*) res;
  for (i=0; i<req; i++) clr[i] = 0;
  return res;
}
#endif // USE_NANO_MALLOC
