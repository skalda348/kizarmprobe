#include <stdlib.h>


void* operator new (unsigned int size) {
  return calloc(1,size);
}
void operator delete (void* p) {
  free (p);
}

void* operator new[] (unsigned int size) {
  return calloc(1,size);
}
void operator delete[] (void* p) {
  free (p);
}

