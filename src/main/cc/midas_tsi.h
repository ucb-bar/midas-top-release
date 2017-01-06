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
  virtual bool data_available();
  virtual void send_word(uint32_t word);
  virtual uint32_t recv_word();
  virtual bool done() {
    return midas_fesvr_t::done();
  }
  virtual int exit_code() {
    return midas_fesvr_t::exit_code();
  }

 protected:
  virtual void idle();

 private:
  midas_context_t host;
  midas_context_t* target;
  std::deque<uint32_t> in_data;
  std::deque<uint32_t> out_data;
  size_t idle_counts;

  virtual void read(uint32_t* data, size_t len);
  virtual void write(const uint32_t* data, size_t len);

  static int host_thread(void *tsi);
};

#endif // __MIDAS_TSI_H
