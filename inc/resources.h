#ifndef RESOURCES_H
#define RESOURCES_H

#include <stdint.h>

#ifdef __cplusplus
  extern "C" {
#endif //__cplusplus

  extern const char tdesc_cortex_m  [];
  extern const char tdesc_cortex_mf [];
  
  extern const char       stm32f1_xml_memory_map [];
  extern const char       stm32hd_xml_memory_map [];
  extern const uint32_t   stm32f1_flash_write_stub [];
  extern const uint32_t   stm32f1_flash_write_stub_size;

  extern const uint32_t   stm32f4_flash_write_stub [];
  extern const uint32_t   stm32f4_flash_write_stub_size;

#ifdef __cplusplus
  };
#endif //__cplusplus
#endif // RESOURCES_H
