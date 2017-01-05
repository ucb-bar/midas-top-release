#ifndef __MIDAS_TOP_H
#define __MIDAS_TOP_H

#include "simif.h"
#include "serial.h"
#include "midas_tsi.h"
#include "endpoints/sim_mem.h"

class midas_top_t: virtual simif_t
{
public:
  midas_top_t(int argc, char** argv): mem(this, argc, argv), serial(this) {
    std::vector<std::string> args(argv + 1, argv + argc);
    tsi = new midas_tsi_t(args);
    max_cycles = -1;
    for (auto &arg: args) {
      if (arg.find("+max-cycles=") == 0) {
        max_cycles = atoi(arg.c_str()+12);
      }
    }
  }

  ~midas_top_t() {
    delete tsi;
  }

  void run(size_t step_size) {
    // set_tracelen(TRACE_MAX_LEN);
    mem.init();
    // Assert reset T=0 -> 5
    target_reset(0, 5);

    uint64_t start_time = timestamp();

    size_t delta = 0;
    serial_data_t data;
    data.in.valid = false;
    data.out.ready = false;

    do {
      serial.recv(data);
      if (data.in.fire() || !data.in.valid) {
        if ((data.in.valid = tsi->data_available()))
          data.in.bits = tsi->recv_word();
      }
      if (data.out.fire()) tsi->send_word(data.out.bits);
      data.out.ready = data.out.valid;
      serial.send(data);
      tsi->switch_to_host();
      if (data.in.valid || data.out.valid) {
        step(1, false);
        if (--delta == 0) delta = step_size;
      } else {
        step(delta, false);
        delta = step_size;
      }
      while (!done() || !mem.done()) mem.tick();
    } while (!tsi->done() && cycles() <= max_cycles);

    uint64_t end_time = timestamp();
    double sim_time = (double) (end_time - start_time) / 1000000.0;
    double sim_speed = (double) cycles() / sim_time / 1000.0;
    if (sim_speed > 1000.0) {
      fprintf(stderr, "time elapsed: %.1f s, simulation speed = %.2f MHz\n", sim_time, sim_speed / 1000.0);
    } else {
      fprintf(stderr, "time elapsed: %.1f s, simulation speed = %.2f KHz\n", sim_time, sim_speed);
    }
    int exitcode = tsi->exit_code();
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
  sim_mem_t mem;
  serial_t serial;
  midas_tsi_t *tsi;
  uint64_t max_cycles;
};

#endif // __MIDAS_TOP_H
