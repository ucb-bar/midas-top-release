#ifndef __SERIAL_H
#define __SERIAL_H

#include "simif.h"

struct serial_data_t {
  struct {
    uint32_t bits;
    bool valid;
    bool ready;
    bool fire() { return valid && ready; }
  } in, out;
};

class serial_t
{
public:
  serial_t(simif_t* sim): sim(sim) { }

  virtual void send(serial_data_t& data) {
    if (data.in.valid) sim->poke(io_serial_in_bits, data.in.bits);
    sim->poke(io_serial_in_valid, data.in.valid);
    sim->poke(io_serial_out_ready, data.out.ready);
  }
  virtual void recv(serial_data_t& data) {
    data.in.ready = sim->peek(io_serial_in_ready);
    data.out.valid = sim->peek(io_serial_out_valid);
    if (data.out.valid) data.out.bits = sim->peek(io_serial_out_bits);
  }

private:
  simif_t* sim;
};

#endif // __SERIAL_H
