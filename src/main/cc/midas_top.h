#ifndef __MIDAS_TOP_H
#define __MIDAS_TOP_H

#include "simif.h"
#include "endpoints/sim_mem.h"
#include "endpoints/serial.h"
#include "fesvr/fesvr_proxy.h"
#ifdef SIMPLE_NIC
#include "endpoints/switch.h"
#endif

class midas_top_t: virtual simif_t
{
public:
  midas_top_t(int argc, char** argv, fesvr_proxy_t* fesvr);
  ~midas_top_t() { }

  void run(size_t step_size);
  void loadmem();

private:
  std::vector<endpoint_t*> endpoints;
  fesvr_proxy_t* fesvr;
#ifdef SIMPLE_NIC
  switch_t sw;
#endif
  uint64_t max_cycles;

  void loop(size_t step_size);
};

#endif // __MIDAS_TOP_H
