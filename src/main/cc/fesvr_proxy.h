#ifndef __FESVR_PROXY_H
#define __FESVR_PROXY_H

struct fesvr_mem_t {
  fesvr_mem_t(): wr(false), addr(0) { }
  fesvr_mem_t(bool wr, size_t addr): wr(wr), addr(addr) { }
  bool wr;
  size_t addr;
};

struct fesvr_loadmem_t {
  fesvr_loadmem_t(): addr(0), size(0) { }
  fesvr_loadmem_t(size_t addr, size_t size): addr(addr), size(size) { }
  size_t addr;
  size_t size;
};

class fesvr_proxy_t
{
public:
  virtual bool recv_mem_req(fesvr_mem_t& req) = 0;
  virtual uint64_t recv_mem_wdata() = 0;
  virtual void send_mem_rdata(uint64_t) = 0;

  virtual bool recv_loadmem_req(fesvr_loadmem_t& req) = 0;
  virtual void recv_loadmem_data(void* buf, size_t len) = 0;

  virtual void tick() = 0;
  virtual bool busy() = 0;
  virtual bool done() = 0;
  virtual bool started() = 0;
  virtual int exit_code() = 0;
};

#endif // __FESVR_PROXY_H
