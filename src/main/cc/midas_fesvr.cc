#include "midas_fesvr.h"
#include <fesvr/configstring.h>

#define NHARTS_MAX 16

midas_fesvr_t::midas_fesvr_t(const std::vector<std::string>& args) : htif_t(args)
{
}

midas_fesvr_t::~midas_fesvr_t(void)
{
}

// Interrupt each core to make it start executing
void midas_fesvr_t::reset()
{
  uint32_t one = 1;
  addr_t ipis[NHARTS_MAX];
  int ncores = get_ipi_addrs(ipis);

  if (ncores == 0) {
      fprintf(stderr, "ERROR: No cores found\n");
      abort();
  }

  for (int i = 0; i < ncores; i++)
    write_chunk(ipis[i], sizeof(uint32_t), &one);
}

void midas_fesvr_t::push_addr(addr_t addr)
{
  uint32_t data[FESVR_ADDR_CHUNKS];
  for (int i = 0; i < FESVR_ADDR_CHUNKS; i++) {
    data[i] = addr & 0xffffffff;
    addr = addr >> 32;
  }
  write(data, FESVR_ADDR_CHUNKS);
}

void midas_fesvr_t::push_len(size_t len)
{
  uint32_t data[FESVR_LEN_CHUNKS];
  for (int i = 0; i < FESVR_LEN_CHUNKS; i++) {
    data[i] = len & 0xffffffff;
    len = len >> 32;
  }
  write(data, FESVR_LEN_CHUNKS);
}

void midas_fesvr_t::read_chunk(addr_t taddr, size_t nbytes, void* dst)
{
  const uint32_t cmd = FESVR_CMD_READ;
  uint32_t *result = static_cast<uint32_t*>(dst);
  size_t len = nbytes / sizeof(uint32_t);

  write(&cmd, 1);
  push_addr(taddr);
  push_len(len - 1);

  read(result, len);
}

void midas_fesvr_t::write_chunk(addr_t taddr, size_t nbytes, const void* src)
{
  const uint32_t cmd = FESVR_CMD_WRITE;
  const uint32_t *src_data = static_cast<const uint32_t*>(src);
  size_t len = nbytes / sizeof(uint32_t);

  write(&cmd, 1);
  push_addr(taddr);
  push_len(len - 1);

  write(src_data, len);
}

int midas_fesvr_t::get_ipi_addrs(addr_t *ipis)
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

