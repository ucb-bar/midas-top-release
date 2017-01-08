#include "simpleswitchcore.h"

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
  switch_t(simif_t* sim): sim(sim), sw(init_switch(1)) { }

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
    transport_value value;
    value.data = data.out.value.data;
    value.is_last = data.out.value.is_last;
    value.is_empty = data.out.value.is_empty;
    step_switch_net(sw, &value);
    data.in.value.data = value.data;
    data.in.value.is_last = value.is_last;
    data.in.value.is_empty = value.is_empty;
  }

private:
  simif_t* sim;
  simpleswitch* sw;
};
