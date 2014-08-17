#ifndef UTILS_H
#define UTILS_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif //__cpluslpus

uint8_t FromHex (uint8_t n);
uint8_t toHex   (uint8_t n);

unsigned char * hexify   (char *hex, const unsigned char *buf, int size);
unsigned char * unhexify (unsigned char *buf, const unsigned char *hex, int size);

uint32_t        crc32_calc  (uint32_t crc, uint8_t data);

#ifdef __cplusplus
};
#endif //__cplusplus

#endif // UTILS_H
