#ifndef __MIDAS_TSI_H
#define __MIDAS_TSI_H

#include "midas_context.h"
#include "midas_fesvr.h"
#include "fesvr_proxy.h"

class midas_tsi_t : public fesvr_proxy_t, public midas_fesvr_t
{
 public:
  midas_tsi_t(const std::vector<std::string>& args);
  virtual ~midas_tsi_t();
  virtual void tick();

  virtual bool recv_mem_req(fesvr_mem_t& req);
  virtual uint64_t recv_mem_wdata();
  virtual void send_mem_rdata(uint64_t);

  virtual bool recv_loadmem_req(fesvr_loadmem_t& req);
  virtual void recv_loadmem_data(void* buf, size_t len);

  virtual bool done() {
    return midas_fesvr_t::done();
  }
  virtual bool started() {
    return midas_fesvr_t::started();
  }
  virtual int exit_code() {
    return midas_fesvr_t::exit_code();
  }

 protected:
  virtual void idle();

 private:
  midas_context_t host;
  midas_context_t* target;
  std::deque<fesvr_mem_t> mem_reqs;
  std::deque<uint64_t> wdata;
  std::deque<uint64_t> rdata;
  std::deque<fesvr_loadmem_t> loadmem_reqs;
  std::deque<char> loadmem_data;
  size_t idle_counts;

  virtual uint64_t read_mem(addr_t addr);
  virtual void write_mem(addr_t addr, uint64_t data);
  virtual void load_mem(addr_t addr, size_t nbytes, const void* src);

  static int host_thread(void *tsi);
};

#endif // __MIDAS_TSI_H
