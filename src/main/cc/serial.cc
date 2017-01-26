#include "serial.h"

serial_t::serial_t(simif_t* sim, fesvr_proxy_t* fesvr):
  sim(sim), fesvr(fesvr), state(s_idle)
{
  assert(INPUT_CHUNKS[io_fesvr_addr] == 1);
  data.in.valid = false;
  data.out.ready = false;
}

void serial_t::send() {
  if (data.in.valid) {
    sim->poke(io_fesvr_addr, data.in.addr);
    if (data.in.wr) sim->poke(io_fesvr_wdata, data.in.wdata);
  }
  data_t metaIn = 0;
  metaIn |= (data.in.wr & 0x1) << 2;
  metaIn |= (data.in.valid & 0x1) << 1;
  metaIn |= (data.out.ready & 0x1);
  sim->poke(io_fesvr_metaIn, metaIn);
}

void serial_t::recv() {
  data_t metaOut = sim->peek(io_fesvr_metaOut);
  data.in.ready = metaOut & 0x1;
  data.out.valid = (metaOut >> 1) & 0x1;
  data.out.has_data = (metaOut >> 2) & 0x1;
  if (data.out.valid && data.out.has_data) {
    sim->peek(io_fesvr_rdata, data.out.rdata);
  }
}

void serial_t::tick() {
  data.out.ready = state == s_grant;
  fesvr_mem_t req;
  switch (state) {
    case s_idle:
      if ((data.in.valid = fesvr->recv_mem_req(req))) {
        data.in.addr = req.addr;
        if ((data.in.wr = req.wr))
          data.in.wdata = biguint_t(fesvr->recv_mem_wdata());
        state = s_acquire;
      }
      break;
    case s_acquire:
      if (data.in.ready) {
        data.in.valid = false;
        state = s_grant;
      }
      break;
    case s_grant:
      if (data.out.valid) {
        if (data.out.has_data)
          fesvr->send_mem_rdata(data.out.rdata.uint64());
        state = s_idle;
      }
      break;
  }
  fesvr->tick(); 
}

bool serial_t::busy() {
  return fesvr->busy() || data.in.valid || data.out.valid || state != s_idle;
}
