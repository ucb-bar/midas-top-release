#include "midas_fesvr.h"
#include <fesvr/configstring.h>

#define NHARTS_MAX 16

midas_fesvr_t::midas_fesvr_t(const std::vector<std::string>& args) : htif_t(args)
{
  is_loadmem = false;
  is_started = false;
}

midas_fesvr_t::~midas_fesvr_t(void)
{
}

// Interrupt each core to make it start executing
void midas_fesvr_t::reset()
{
  uint32_t one = 1;
  reg_t ipis[NHARTS_MAX];
  int ncores = get_ipi_addrs(ipis);

  if (ncores == 0) {
      fprintf(stderr, "ERROR: No cores found\n");
      abort();
  }

  for (int i = 0; i < ncores; i++)
    write_chunk(ipis[i], sizeof(uint32_t), &one);

  is_started = true;
  idle();
}

int midas_fesvr_t::get_ipi_addrs(reg_t *ipis)
{
  const char *cfgstr = config_string.c_str();
  query_result res;
  char key[32];

  for (int core = 0; ; core++) {
    snprintf(key, sizeof(key), "core{%d{0{ipi", core);
    res = query_config_string(cfgstr, key);
    if (res.start == NULL)
      return core;
    ipis[core] = get_uint(res);
  }
}

void midas_fesvr_t::read_chunk(addr_t taddr, size_t nbytes, void* dst)
{
  for (size_t off = 0 ; off < nbytes ; off += sizeof(uint64_t)) {
    uint64_t data = read_mem(taddr + off);
    memcpy((char*)dst + off, &data, std::min(sizeof(uint64_t), nbytes - off));
  }
}

void midas_fesvr_t::write_chunk(addr_t taddr, size_t nbytes, const void* src)
{
  if (is_loadmem) {
    load_mem(taddr, nbytes, src);
  } else {
    for (size_t off = 0 ; off < nbytes ; off += sizeof(uint64_t)) {
      uint64_t data;
      memcpy(&data, (const char*)src + off, std::min(sizeof(uint64_t), nbytes - off));
      write_mem(taddr + off, data);
    }
  }
}
