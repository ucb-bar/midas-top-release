#ifndef __TSI_H
#define __TSI_H

#ifdef ZYNQ // TODO: it's awkward to have different tsi's

#include <fesvr/tsi.h>

class tsi_midas_t : public tsi_t
{
 public:
  tsi_midas_t(const std::vector<std::string>& target_args);
 protected:
  void idle() override;
 private:
  size_t idle_counts;
};

#else

#include <fesvr/htif.h>
#include "context.h"

#include <string>
#include <vector>
#include <deque>
#include <stdint.h>

#define TSI_CMD_READ 0
#define TSI_CMD_WRITE 1

#define TSI_ADDR_CHUNKS 2
#define TSI_LEN_CHUNKS 2

class tsi_midas_t : public htif_t
{
 public:
  tsi_midas_t(const std::vector<std::string>& target_args);
  virtual ~tsi_midas_t();

  bool data_available();
  void send_word(uint32_t word);
  uint32_t recv_word();
  void switch_to_host();

  uint32_t in_bits() { return in_data.front(); }
  bool in_valid() { return !in_data.empty(); }
  bool out_ready() { return true; }
  void tick(bool out_valid, uint32_t out_bits, bool in_ready);

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
  context_t host;
  context_t* target;
  std::deque<uint32_t> in_data;
  std::deque<uint32_t> out_data;
  size_t idle_counts;

  void push_addr(addr_t addr);
  void push_len(size_t len);

  static int host_thread(void *tsi);
};

#endif // ZYNQ

#endif // __TSI_H
