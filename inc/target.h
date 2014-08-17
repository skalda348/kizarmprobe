#ifndef TARGET_H
#define TARGET_H
#include <stdint.h>
#include "adiv5apdp.h"
#include "gdbserver.h"

class Target {

  public:
    Target (GdbServer* s);
    virtual ~Target () {};
    /* Notify controlling debugger if target is lost */
    // target_destroy_callback destroy_callback;
    /* Attach/Detach funcitons */
    virtual void remove         (void) = 0;
    virtual bool attach         (void) = 0;
    virtual void detach         (void) = 0;
    virtual const char* getName (void) = 0;

    /* Register access functions */
    int get_regs_size   (void) {return regs_size;};
    virtual int regs_read       (void       *data) = 0;
    virtual int regs_write      (const void *data) = 0;

    virtual uint32_t pc_read    (void) = 0;
    virtual int      pc_write   (const uint32_t val) = 0;

    /* Halt/resume functions */
    virtual void reset          (void) = 0;
    virtual void halt_request   (void) = 0;
    virtual int  halt_wait      (void) = 0;
    virtual void halt_resume    (bool step) = 0;

    /* Break-/watchpoint functions */
    virtual int set_hw_bp       (uint32_t addr) = 0;
    virtual int clear_hw_bp     (uint32_t addr) = 0;

    virtual int set_hw_wp       (uint8_t type, uint32_t addr, uint8_t len) = 0;
    virtual int clear_hw_wp     (uint8_t type, uint32_t addr, uint8_t len) = 0;

    virtual int check_hw_wp     (uint32_t *addr) = 0;


    /* Host I/O support */
    virtual int  hostio_request (void) = 0;
    virtual void hostio_reply   (int32_t retcode, uint32_t errcode) = 0;

    /* Memory access functions */
    int mem_read_words  (uint32_t *dest, uint32_t        src, int len);
    int mem_write_words (uint32_t  dest, const uint32_t *src, int len);
    int mem_read_bytes  (uint8_t  *dest, uint32_t        src, int len);
    int mem_write_bytes (uint32_t  dest, const uint8_t  *src, int len);
    
    /* Flash memory access functions */
    virtual int flash_erase     (uint32_t addr, int len);
    virtual int flash_write     (uint32_t dest, const uint8_t *src, int len);
    
    int      check_error        (void);
    uint32_t generic_crc32      (uint32_t base, int len);

    virtual bool probe     (void);
    GdbServer *  getServer (void);
  private:
    GdbServer *       gdb;
  public:       // data jsou veřejná kvůli jednoduchosti.
    ADIv5APDP         apdp;

    bool                attached;
    const char *        xml_mem_map;
    /* Register access */
    int                 regs_size;
    const char *        tdesc;
    /* target-defined options */
    unsigned            target_options;
    uint32_t            idcode;
    //Target     *        next;         // Multitarget je pro SWD celkem zbytečný.
};

#endif // TARGET_H
