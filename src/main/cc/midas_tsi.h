#ifndef __TSI_H
#define __TSI_H

#include <fesvr/htif.h>
#include "midas_context.h"

#include <string>
#include <vector>
#include <deque>
#include <stdint.h>

#define TSI_CMD_READ 0
#define TSI_CMD_WRITE 1

#define TSI_ADDR_CHUNKS 2
#define TSI_LEN_CHUNKS 2

class midas_tsi_t : public htif_t
{
 public:
  midas_tsi_t(const std::vector<std::string>& args);
  virtual ~midas_tsi_t();

  bool data_available();
  void send_word(uint32_t word);
  uint32_t recv_word();
  void switch_to_host();

 protected:
  void reset() override;
  void idle() override;
  void read_chunk(addr_t taddr, size_t nbytes, void* dst) override;
  void write_chunk(addr_t taddr, size_t nbytes, const void* src) override;
  void switch_to_target();

  size_t chunk_align() { return 4; }
  size_t chunk_max_size() { return 1024; }

  int get_ipi_addrs(addr_t *addrs);

 private:
  midas_context_t host;
  midas_context_t* target;
  std::deque<uint32_t> in_data;
  std::deque<uint32_t> out_data;
  size_t idle_counts;

  void push_addr(addr_t addr);
  void push_len(size_t len);

  static int host_thread(void *tsi);
};

#endif // __TSI_H
