#ifndef __SERIAL_H
#define __SERIAL_H

#include "endpoints/endpoint.h"
#include "fesvr/fesvr_proxy.h"

struct serial_data_t {
  struct {
    uint64_t bits;
    bool valid;
    bool ready;
    bool fire() { return valid && ready; }
  } in;
  struct {
    uint64_t bits;
    bool ready;
    bool valid;
    bool fire() { return valid && ready; }
  } out;
};

class serial_t: public endpoint_t
{
public:
  serial_t(simif_t* sim, fesvr_proxy_t* fesvr);
  void send(serial_data_t&);
  void recv(serial_data_t&);
  void work();
  virtual void tick() { }
  virtual bool done() { return read(SERIALWIDGET_0(done)); }
  virtual bool stall() { return false; }

private:
  fesvr_proxy_t* fesvr;
};

#endif // __SERIAL_H
