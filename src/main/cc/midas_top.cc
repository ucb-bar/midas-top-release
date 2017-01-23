#include "midas_top.h"

midas_top_t::midas_top_t(int argc, char** argv, fesvr_proxy_t* fesvr):
  serial(this, fesvr), mem(this, argc, argv), fesvr(fesvr)
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

// condition functions

bool not_started(fesvr_proxy_t* fesvr) {
  return !fesvr->started();
}

bool not_done(fesvr_proxy_t* fesvr) {
  return !fesvr->done();
}

//

void midas_top_t::loadmem() {
  fesvr_loadmem_t loadmem; 
  while (fesvr->recv_loadmem_req(loadmem)) {
    assert(loadmem.size <= 1024);
    static char buf[1024]; // This should be enough...
    fesvr->recv_loadmem_data(buf, loadmem.size);
#ifdef LOADMEM
    const size_t mem_data_bytes = MEM_DATA_CHUNK * sizeof(data_t);
#define WRITE_MEM(addr, src) \
    biguint_t data((uint32_t*)(src), mem_data_bytes / sizeof(uint32_t)); \
    write_mem(addr, data)
#else
    const size_t mem_data_bytes = MEM_DATA_BITS / 8;
#define WRITE_MEM(addr, src) \
    mem.write_mem(addr, src)
#endif
    for (size_t off = 0 ; off < loadmem.size ; off += mem_data_bytes) {
      WRITE_MEM(loadmem.addr + off, buf + off);
    }
  }
}

void midas_top_t::loop(size_t step_size, bool (*cond)(fesvr_proxy_t*)) {
  size_t delta = step_size;

  do {
    serial.recv();
    serial.tick();
    loadmem();
    serial.send();

#ifdef SIMPLE_NIC
    sw.recv();
    sw.tick();
    sw.send();
#endif
    if (serial.busy()
#ifdef SIMPLE_NIC
        || sw.busy()
#endif
       ) {
      step(1, false);
      if (--delta == 0) delta = step_size;
    } else {
      step(delta, false);
      delta = step_size;
    }
    while (!done() || !mem.done()) mem.tick();
  } while (cond(fesvr) && cycles() <= max_cycles);
}

void midas_top_t::run(size_t step_size) {
  // set_tracelen(TRACE_MAX_LEN);
  mem.init();
  // Assert reset T=0 -> 5
  target_reset(0, 5);

  uint64_t start_time = timestamp();

  loop(1, not_started);
  // align cycles for snapshotting
  step(cycles() % step_size, false);
  while(!done() || !mem.done()) mem.tick();
  //
  loop(step_size, not_done); 

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
