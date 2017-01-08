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
  midas_top_t(int argc, char** argv, fesvr_proxy_t* fesvr):
    serial(this), mem(this, argc, argv), fesvr(fesvr)
#ifdef SIMPLE_NIC
    , sw(this)
#endif
  {
    std::vector<std::string> args(argv + 1, argv + argc);
    max_cycles = -1;
    for (auto &arg: args) {
      if (arg.find("+max-cycles=") == 0) {
        max_cycles = atoi(arg.c_str()+12);
      }
    }
  }

  ~midas_top_t() { }

  void run(size_t step_size) {
    // set_tracelen(TRACE_MAX_LEN);
    mem.init();
    // Assert reset T=0 -> 5
    target_reset(0, 5);

    uint64_t start_time = timestamp();

    size_t delta = step_size;
    serial_data_t data;
    data.in.valid = false;
    data.out.ready = false;
#ifdef SIMPLE_NIC
    switch_data_t sw_data;
    sw_data.in.value.is_empty = false;
    sw_data.out.ready = true;
#endif

    do {
      serial.recv(data);
      if (data.in.fire() || !data.in.valid) {
        data.in.valid = fesvr->data_available();
        if (data.in.valid) data.in.bits = fesvr->recv_word();
      }
      if (data.out.fire()) fesvr->send_word(data.out.bits);
      data.out.ready = data.out.valid;
      serial.send(data);
      fesvr->tick();
#ifdef SIMPLE_NIC
      sw.recv(sw_data);
      sw.tick(sw_data);
      sw.send(sw_data);
#endif
      if (data.in.valid || data.out.valid
#ifdef SIMPLE_NIC
          || true // || !sw_data.in.value.is_empty || !sw_data.out.value.is_empty
#endif
         ) {
        step(1, false);
        if (--delta == 0) delta = step_size;
      } else {
        step(delta, false);
        delta = step_size;
      }
      while (!done() || !mem.done()) mem.tick();
    } while (!fesvr->done() && cycles() <= max_cycles);

    uint64_t end_time = timestamp();
    double sim_time = diff_secs(end_time, start_time);
    double sim_speed = ((double) cycles()) / (sim_time * 1000.0);
    if (sim_speed > 1000.0) {
      fprintf(stderr, "time elapsed: %.1f s, simulation speed = %.2f MHz\n", sim_time, sim_speed / 1000.0);
    } else {
      fprintf(stderr, "time elapsed: %.1f s, simulation speed = %.2f KHz\n", sim_time, sim_speed);
    }
    int exitcode = fesvr->exit_code();
    if (exitcode) {
      fprintf(stderr, "*** FAILED *** (code = %d) after %" PRIu64 " cycles\n", exitcode, cycles());
    } else if (cycles() > max_cycles) {
      fprintf(stderr, "*** FAILED *** (timeout) after %" PRIu64 " cycles\n", cycles());
    } else {
      fprintf(stderr, "*** PASSED *** after %" PRIu64 " cycles\n", cycles());
    }
    expect(!exitcode, NULL);
  }

private:
  serial_t serial;
  sim_mem_t mem;
  fesvr_proxy_t* fesvr;
#ifdef SIMPLE_NIC
  switch_t sw;
#endif
  uint64_t max_cycles;
};

#endif // __MIDAS_TOP_H
