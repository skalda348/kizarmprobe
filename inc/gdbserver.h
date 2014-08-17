#ifndef GDBSERVER_H
#define GDBSERVER_H

#include "baselayer.h"
#include "monitor.h"
#include "locker.h"

class Target;
class Monitor;

class GdbServer : public BaseLayer {

  public:
    GdbServer();
   //~GdbServer();
    void Fini            (void);
    void Scan            (void);
    void Polling         (void);
    
    uint32_t Up          (char *data, uint32_t len);
    void gdb_putpacket_f (const char *fmt, ...);
    void gdb_out         (const char *buf);
    //int  gdb_outf        (const char *fmt, ...);
  protected:
    void handle_q_packet (char *packet, int len);
    void handle_v_packet (char *packet, int len);
    void handle_z_packet (char *packet, int len);
    void handle_q_string_reply (const char *str, const char *param);
    
    void gdb_putpacket   (const char *packet, int size);
    void gdb_putpacketz  (const char *packet);
    bool target_check    (void);
    /// Probe for @param n new target. If failed, n is delete, else n used as target.
    bool probe           (Target* n);
  public:
    Target  * target;
    Monitor   mon;
  private:
    int  size;
    int  signal;
    bool single_step;
    volatile bool active;
    char last_activity;
    Locker  lock;
};

#endif // GDBSERVER_H
