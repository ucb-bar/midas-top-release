#ifndef __SERIAL_H
#define __SERIAL_H

#include "simif.h"
#include "fesvr_proxy.h"

class serial_t
{
public:
  serial_t(simif_t* sim, fesvr_proxy_t* fesvr);
  void send();
  void recv();
  void tick();
  bool busy();

private:
  simif_t* sim;
  fesvr_proxy_t* fesvr;
  enum { s_idle, s_acquire, s_grant } state;
  struct {
    struct {
      data_t addr;
      biguint_t wdata;
      bool wr;
      bool valid;
      bool ready;
      bool fire() { return valid && ready; }
    } in;
    struct {
      biguint_t rdata;
      bool has_data;
      bool ready;
      bool valid;
      bool fire() { return valid && ready; }
    } out;
  } data;
};

#endif // __SERIAL_H
