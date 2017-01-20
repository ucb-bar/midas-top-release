#ifndef __MIDAS_FESVR_H
#define __MIDAS_FESVR_H

#include <fesvr/htif.h>
#include <string>
#include <vector>
#include <deque>
#include <stdint.h>
#include "fesvr_proxy.h"

#define FESVR_CMD_READ 0
#define FESVR_CMD_WRITE 1

#define FESVR_ADDR_CHUNKS 2
#define FESVR_LEN_CHUNKS 2

class midas_fesvr_t : public htif_t
{
 public:
  midas_fesvr_t(const std::vector<std::string>& args);
  virtual ~midas_fesvr_t();
  virtual void wait() = 0;
  bool started() { return is_started; }

 protected:
  virtual void idle();
  virtual void reset();
  virtual void load_program() {
    is_loadmem = true;
    htif_t::load_program();
    is_loadmem = false;
  }

  virtual void read_chunk(reg_t taddr, size_t nbytes, void* dst);
  virtual void write_chunk(reg_t taddr, size_t nbytes, const void* src);

  size_t chunk_align() { return sizeof(uint64_t); }
  size_t chunk_max_size() { return 1024; }

  int get_ipi_addrs(reg_t *addrs);

  std::deque<fesvr_mem_t> mem_reqs;
  std::deque<uint64_t> wdata;
  std::deque<uint64_t> rdata;
  std::deque<fesvr_loadmem_t> loadmem_reqs;
  std::deque<char> loadmem_data;

 private:
  bool is_loadmem;
  bool is_started;
  size_t idle_counts;

  virtual uint64_t read_mem(addr_t addr);
  virtual void write_mem(addr_t addr, uint64_t data);
  virtual void load_mem(addr_t addr, size_t nbytes, const void* src);
};

#endif // __MIDAS_FESVR_H
