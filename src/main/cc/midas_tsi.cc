#include "midas_tsi.h"
#include "biguint.h"

int midas_tsi_t::host_thread(void *arg)
{
  midas_tsi_t *tsi = static_cast<midas_tsi_t*>(arg);
  tsi->run();

  while (true)
    tsi->target->switch_to();

  return 0;
}

midas_tsi_t::midas_tsi_t(const std::vector<std::string>& args): midas_fesvr_t(args)
{
  target = midas_context_t::current();
  host.init(host_thread, this);
}

midas_tsi_t::~midas_tsi_t()
{
}

void midas_tsi_t::tick()
{
  host.switch_to();
}

void midas_tsi_t::wait()
{
  target->switch_to();
}

bool midas_tsi_t::recv_mem_req(fesvr_mem_t& req) {
  if (mem_reqs.empty()) return false;
  auto r = mem_reqs.front();
  req.wr = r.wr;
  req.addr = r.addr;
  mem_reqs.pop_front();
  return true;
}

uint64_t midas_tsi_t::recv_mem_wdata()
{
  uint64_t data = wdata.front();
  wdata.pop_front();
  return data;
}

void midas_tsi_t::send_mem_rdata(uint64_t data)
{
  rdata.push_back(data);
}

bool midas_tsi_t::recv_loadmem_req(fesvr_loadmem_t& loadmem) {
  if (loadmem_reqs.empty()) return false;
  auto r = loadmem_reqs.front();
  loadmem.addr = r.addr;
  loadmem.size = r.size;
  loadmem_reqs.pop_front();
  return true;
}

void midas_tsi_t::recv_loadmem_data(void* buf, size_t len) {
  std::copy(loadmem_data.begin(), loadmem_data.begin() + len, (char*)buf);
  loadmem_data.erase(loadmem_data.begin(), loadmem_data.begin() + len);
}
