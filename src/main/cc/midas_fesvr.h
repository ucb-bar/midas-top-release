#ifndef __MIDAS_FESVR_H
#define __MIDAS_FESVR_H

#include <fesvr/htif.h>
#include <string>
#include <vector>
#include <deque>
#include <stdint.h>

#define FESVR_CMD_READ 0
#define FESVR_CMD_WRITE 1

#define FESVR_ADDR_CHUNKS 2
#define FESVR_LEN_CHUNKS 2

class midas_fesvr_t : public htif_t
{
 public:
  midas_fesvr_t(const std::vector<std::string>& args);
  virtual ~midas_fesvr_t();

 protected:
  virtual void reset();
  virtual void read_chunk(addr_t taddr, size_t nbytes, void* dst);
  virtual void write_chunk(addr_t taddr, size_t nbytes, const void* src);

  size_t chunk_align() { return 4; }
  size_t chunk_max_size() { return 1024; }

  int get_ipi_addrs(addr_t *addrs);

 private:
  void push_addr(addr_t addr);
  void push_len(size_t len);

  virtual void read(uint32_t* data, size_t len) = 0;
  virtual void write(const uint32_t* data, size_t len) = 0;
};

#endif // __MIDAS_FESVR_H
