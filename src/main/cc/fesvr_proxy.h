#ifndef __FESVR_PROXY_H
#define __FESVR_PROXY_H

class fesvr_proxy_t
{
public:
  virtual bool data_available() = 0;
  virtual uint32_t recv_word() = 0;
  virtual void send_word(uint32_t word) = 0;
  virtual void tick() = 0;
  virtual bool done() = 0;
  virtual int exit_code() = 0;
};

#endif // __FESVR_PROXY_H
