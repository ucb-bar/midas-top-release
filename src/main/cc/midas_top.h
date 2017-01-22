#ifndef __MIDAS_TOP_H
#define __MIDAS_TOP_H

#include "simif.h"
#include "endpoints/sim_mem.h"
#include "serial.h"
#include "fesvr_proxy.h"
#ifdef SIMPLE_NIC
#include "switch.h"
#endif

class midas_top_t: virtual simif_t
{
public:
  midas_top_t(int argc, char** argv, fesvr_proxy_t* fesvr);
  ~midas_top_t() { }

  void run(size_t step_size);
  void loadmem();

private:
  serial_t serial;
  sim_mem_t mem;
  fesvr_proxy_t* fesvr;
#ifdef SIMPLE_NIC
  switch_t sw;
  switch_data_t sw_data;
#endif
  uint64_t max_cycles;

  void loop(size_t step_size, bool (*cond)(fesvr_proxy_t*));
};

#endif // __MIDAS_TOP_H
