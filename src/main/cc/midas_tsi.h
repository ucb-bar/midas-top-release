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
  virtual void wait();
  virtual void tick();

  virtual bool recv_mem_req(fesvr_mem_t& req);
  virtual uint64_t recv_mem_wdata();
  virtual void send_mem_rdata(uint64_t);

  virtual bool recv_loadmem_req(fesvr_loadmem_t& loadmem);
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

 private:
  midas_context_t host;
  midas_context_t* target;

  static int host_thread(void *tsi);
};

#endif // __MIDAS_TSI_H
