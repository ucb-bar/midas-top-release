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
  idle_counts = 10;
  for (auto& arg: args) {
    if (arg.find("+idle-counts=") == 0) {
      idle_counts = atoi(arg.c_str()+13);
    }
  }
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

void midas_tsi_t::idle() {
  for (size_t i = 0 ; i < idle_counts ; i++)
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

uint64_t midas_tsi_t::read_mem(addr_t addr) {
  mem_reqs.push_back(fesvr_mem_t(false, addr)); 
  while (rdata.empty()) target->switch_to();
  uint64_t data = rdata.front();
  rdata.pop_front();
  return data;
}

void midas_tsi_t::write_mem(addr_t addr, uint64_t data) {
  mem_reqs.push_back(fesvr_mem_t(true, addr));
  wdata.push_back(data);
}

bool midas_tsi_t::recv_loadmem_req(fesvr_loadmem_t& req) {
  if (loadmem_reqs.empty()) return false;
  auto r = loadmem_reqs.front();
  req.addr = r.addr;
  req.size = r.size;
  loadmem_reqs.pop_front();
  return true;
}

void midas_tsi_t::recv_loadmem_data(void* buf, size_t len) {
  std::copy(loadmem_data.begin(), loadmem_data.begin() + len, (char*)buf);
  loadmem_data.erase(loadmem_data.begin(), loadmem_data.begin() + len);
}

void midas_tsi_t::load_mem(addr_t addr, size_t nbytes, const void* src) {
  loadmem_reqs.push_back(fesvr_loadmem_t(addr, nbytes));
  loadmem_data.insert(loadmem_data.end(), (const char*)src, (const char*)src + nbytes);
}
