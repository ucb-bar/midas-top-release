#ifndef __MIDAS_TOP_H
#define __MIDAS_TOP_H

#include "simif.h"
#include "fesvr/fesvr_proxy.h"
#include "endpoints/endpoint.h"
#include "endpoints/fpga_model.h"

class midas_top_t: virtual simif_t
{
public:
  midas_top_t(int argc, char** argv, fesvr_proxy_t* fesvr);
  ~midas_top_t() { }

  void run(size_t step_size);
  void loadmem();

protected:
   void add_endpoint(endpoint_t* endpoint) {
     endpoints.push_back(endpoint);
   }

private:
  // Memory mapped endpoints bound to software models
  std::vector<endpoint_t*> endpoints;
  // FPGA-hosted models with programmable registers & instrumentation
  std::vector<FpgaModel*> fpga_models;
  fesvr_proxy_t* fesvr;
  uint64_t max_cycles;
  // profile interval: num step_size interations before reading model stats
  // profile_interval = 0 disables model polling
  uint64_t profile_interval = 0;
  void loop(size_t step_size);
};

#endif // __MIDAS_TOP_H
