#include "simif.h"
#include <fesvr/tsi.h>

#include <iostream>
class midas_top_t: virtual simif_t
{
public:
  midas_top_t(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);
    tsi = new tsi_t(args);
    max_cycles = -1;
    latency = 8;
    for (auto &arg: args) {
      if (arg.find("+max-cycles=") == 0) {
        max_cycles = atoi(arg.c_str()+12);
      }
      if (arg.find("+latency=") == 0) {
        latency = atoi(arg.c_str()+9);
      }
    }
  }

  ~midas_top_t() {
    delete tsi;
  }

  void run(size_t step_size = TRACE_MAX_LEN) {
    set_tracelen(step_size);
#ifdef MEMMODEL_0_readLatency
    write(MEMMODEL_0_readMaxReqs, 8);
    write(MEMMODEL_0_writeMaxReqs, 8);
    write(MEMMODEL_0_readLatency, latency);
    write(MEMMODEL_0_writeLatency, latency);
#else
    write(MEMMODEL_0_LATENCY, latency);
#endif
    uint64_t start_time = timestamp();
    tsi->switch_to_host(); // start simulation

    do {
      bool in_valid = false, in_ready;
      bool out_ready = false, out_valid;
      size_t stepped = 0;
      do {
        if ((in_ready = peek(io_serial_in_ready) && in_valid) || !in_valid) {
          if (tsi->data_available()) {
            poke(io_serial_in_bits, tsi->recv_word());
            poke(io_serial_in_valid, in_valid = true);
          } else {
            poke(io_serial_in_valid, in_valid = false);
          }
        }
        if ((out_valid = peek(io_serial_out_valid)) && out_ready) {
          tsi->send_word(peek(io_serial_out_bits));
        }
        poke(io_serial_out_ready, out_ready = true);
        tsi->switch_to_host();
        step(1);
        stepped++;
        if (stepped > step_size) stepped -= step_size;
      } while (in_valid || out_valid);
      poke(io_serial_out_ready, out_ready = false);
      step(step_size - stepped);
    } while (!tsi->done() && cycles() <= max_cycles);

    uint64_t end_time = timestamp();
    double sim_time = (double) (end_time - start_time) / 1000000.0;
    double sim_speed = (double) cycles() / sim_time / 1000000.0;
    fprintf(stdout, "time elapsed: %.1f s, simulation speed = %.2f MHz\n", sim_time, sim_speed);
    int exitcode = tsi->exit_code();
    if (exitcode) {
      fprintf(stdout, "*** FAILED *** (code = %d) after %" PRIu64 " cycles\n", exitcode, cycles());
    } else if (cycles() > max_cycles) {
      fprintf(stdout, "*** FAILED *** (timeout) after %" PRIu64 " cycles\n", cycles());
    } else {
      fprintf(stdout, "*** PASSED *** after %" PRIu64 " cycles\n", cycles());
    }
    expect(!exitcode, NULL);
  }

private:
  tsi_t *tsi;
  uint64_t max_cycles;
  size_t latency;
};
