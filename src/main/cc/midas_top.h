#include "simif.h"
#include "midas_tsi.h"
#include "endpoints/sim_mem.h"

class midas_top_t: virtual simif_t
{
public:
  midas_top_t(int argc, char** argv): mem(this, argc, argv) {
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
    bool in_valid = false;
    bool out_ready = false, out_valid;

    do {
      if ((peek(io_serial_in_ready) && in_valid) || !in_valid) {
        poke(io_serial_in_valid, in_valid = tsi->data_available());
        if (in_valid) poke(io_serial_in_bits, tsi->recv_word());
      }
      if ((out_valid = peek(io_serial_out_valid)) && out_ready) {
        tsi->send_word(peek(io_serial_out_bits));
      }
      tsi->switch_to_host();
      if (in_valid || out_valid) {
        poke(io_serial_out_ready, out_ready = out_valid);
        step(1, false);
        if (--delta == 0) delta = step_size;
      } else {
        poke(io_serial_out_ready, out_ready = false);
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
  midas_tsi_t *tsi;
  uint64_t max_cycles;
};
