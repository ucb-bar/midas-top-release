#ifndef __SWITCH_H
#define __SWITCH_H

#include "simpleswitchcore.h"
#include "channel.h"

#define __SWITCH__

struct switch_data_t {
  struct {
    transport_value value;
    bool ready;
    bool fire() { return !value.is_empty && ready; }
  } in, out;
};

class switch_t
{
public:
  switch_t(simif_t* sim): sim(sim),
#ifdef __SWITCH__
    in("s_in"), out("s_out")
  {
    out.release();
    in.release();
  }
#else
    sw(init_switch(1)) { }
#endif

  ~switch_t() {
#ifdef __SWITCH__
    bool ready;
    do {
      in.acquire();
      if ((ready = in.ready())) {
	in[4] = true; // finish
	in.produce();
      }
      in.release();
    } while(!ready);
#endif
  }

  void send(switch_data_t& data) {
    if (!data.in.value.is_empty) {
      biguint_t bits = biguint_t(data.in.value.is_last) << 64;
      bits |= biguint_t(data.in.value.data);
      sim->poke(io_ext_bitsin_bits, bits);
    }
    sim->poke(io_ext_bitsin_valid, !data.in.value.is_empty);
    sim->poke(io_ext_bitsout_ready, data.out.ready);
  }

  void recv(switch_data_t& data) {
    data.in.ready = sim->peek(io_ext_bitsin_ready);
    data.out.value.is_empty = !sim->peek(io_ext_bitsout_valid);
    if (!data.out.value.is_empty) {
      biguint_t bits;
      sim->peek(io_ext_bitsout_bits, bits);
      data.out.value.is_last = (bits >> 64).uint();
      data.out.value.data = (((uint64_t)bits[1])) << 32 | bits[0];
    }
  }

  void tick(switch_data_t& data) {
#ifdef __SWITCH__
#ifdef __DEBUG__
    if (!data.out.value.is_empty) {
      printf("data out: %llx, last: %d\n", data.out.value.data, data.out.value.is_last);
      fflush(stdout);
    }
#endif
    send_switch(data.out.value);
    recv_switch(data.in.value);
#ifdef __DEBUG__
    if (!data.in.value.is_empty) {
      printf("data in: %llx, last: %d\n", data.in.value.data, data.in.value.is_last);
      fflush(stdout);
    }
#endif
#else
    transport_value value;
    value.data = data.out.value.data;
    value.is_last = data.out.value.is_last;
    value.is_empty = data.out.value.is_empty;
    step_switch_net(sw, &value);
    data.in.value.data = value.data;
    data.in.value.is_last = value.is_last;
    data.in.value.is_empty = value.is_empty;
#endif
  }

private:
  simif_t* sim;
#ifdef __SWITCH__
  channel_t in;
  channel_t out;

  void send_switch(transport_value& value) {
    bool ready;
    do {
      in.acquire();
      if ((ready = in.ready())) {
        in[0] = (value.data >> 32) & 0xffffffff;
        in[1] = value.data & 0xffffffff;
        in[2] = value.is_last;
        in[3] = value.is_empty;
        in[4] = false; // finish
        in.produce();
      }
      in.release();
    } while(!ready);
  }

  void recv_switch(transport_value& value) {
    bool valid;
    do {
      out.acquire();
      if ((valid = out.valid())) {
        value.data = ((uint64_t)out[0] << 32) | out[1];
        value.is_last = out[2];
        value.is_empty = out[3];
	out.consume();
      }
      out.release();
    } while(!valid);
  }

#else
  simpleswitch* sw;
#endif
};

#endif // __SWITCH_H
