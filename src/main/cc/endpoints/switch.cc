#include "switch.h"

// #define __DEBUG__

switch_t::switch_t(simif_t* sim): sim(sim),
#ifdef __SWITCH__
  in("s_in"), out("s_out")
#else
  sw(init_switch(1))
#endif
{
#ifdef __SWITCH__
  out.release();
  in.release();
#endif
  data.in.value.is_empty = true;
  data.out.ready = false;
}

switch_t::~switch_t() {
#ifdef __SWITCH__
  bool ready;
  do {
    in.acquire();
    if ((ready = in.ready())) {
      in[3] = true; // finish
      in.produce();
    }
    in.release();
  } while(!ready);
#endif
}

void switch_t::send() {
  if (!data.in.value.is_empty) {
    biguint_t bits = biguint_t(data.in.value.is_last) << 64;
    bits |= biguint_t(data.in.value.data);
    sim->poke(io_ext_bitsin_bits, bits);
  }
  sim->poke(io_ext_bitsin_valid, !data.in.value.is_empty);
  sim->poke(io_ext_bitsout_ready, data.out.ready);
}

void switch_t::recv() {
  data.in.ready = sim->peek(io_ext_bitsin_ready);
  data.out.value.is_empty = !sim->peek(io_ext_bitsout_valid);
  if (!data.out.value.is_empty) {
    biguint_t bits;
    sim->peek(io_ext_bitsout_bits, bits);
    data.out.value.is_last = (bits >> 64).uint();
    data.out.value.data = (((uint64_t)bits[1])) << 32 | bits[0];
  }
}

void switch_t::tick() {
  data.in.value.is_empty = true;
  if (data.in.ready && (data.out.fire() || data.out.value.is_empty)) {
#ifdef __DEBUG__
    if (!data.out.value.is_empty) {
      fprintf(stderr, "cycle: %d, data out: %llx, last: %d\n",
			sim->cycles(), data.out.value.data, data.out.value.is_last);
      fflush(stderr);
    }
#endif
#ifdef __SWITCH__
    send_switch(data.out.value);
    recv_switch(data.in.value);
#else
    transport_value value;
    value.data = data.out.value.data;
    value.is_last = data.out.value.is_last;
    value.is_empty = data.out.value.is_empty;
    step_switch_net(sw, &value, NULL);
    data.in.value.data = value.data;
    data.in.value.is_last = value.is_last;
    data.in.value.is_empty = value.is_empty;
#endif
#ifdef __DEBUG__
    if (!data.in.value.is_empty) {
      fprintf(stderr, "cycle: %d, data in: %llx, last: %d\n",
			sim->cycles(), data.in.value.data, data.in.value.is_last);
      fflush(stderr);
    }
#endif
  }
  data.out.ready = !data.out.value.is_empty;
}

bool switch_t::busy() {
  return !data.in.value.is_empty || !data.out.value.is_empty;
}

#ifdef __SWITCH__
void switch_t::send_switch(transport_value& value) {
  bool ready;
  do {
    in.acquire();
    if ((ready = in.ready())) {
      in[0] = value.data;
      in[1] = value.is_last;
      in[2] = value.is_empty;
      in[3] = false; // finish
      in.produce();
    }
    in.release();
  } while(!ready);
}

void switch_t::recv_switch(transport_value& value) {
  bool valid;
  do {
    out.acquire();
    if ((valid = out.valid())) {
      value.data = out[0];
      value.is_last = out[1];
      value.is_empty = out[2];
      out.consume();
    }
    out.release();
  } while(!valid);
}
#endif
