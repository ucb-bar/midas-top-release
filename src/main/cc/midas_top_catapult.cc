#include "midas_top_catapult.h"

// #define __DEBUG__

fesvr_channel_t::fesvr_channel_t():
  in("in"), out("out")
{
  in.release();
  out.release();
  fesvr_done = false;
}

fesvr_channel_t::~fesvr_channel_t()
{
}

bool fesvr_channel_t::recv_mem_req(fesvr_mem_t& req) {
  if (mem_reqs.empty()) return false;
  auto r = mem_reqs.front();
  req.wr = r.wr;
  req.addr = r.addr;
  mem_reqs.pop_front();
  return true;
}

uint64_t fesvr_channel_t::recv_mem_wdata()
{
  uint64_t data = wdata.front();
  wdata.pop_front();
  return data;
}


void fesvr_channel_t::send_mem_rdata(uint64_t data)
{
  rdata.push_back(data);
}

bool fesvr_channel_t::recv_loadmem_req(fesvr_loadmem_t& loadmem) {
  if (loadmem_reqs.empty()) return false;
  auto r = loadmem_reqs.front();
  loadmem.addr = r.addr;
  loadmem.size = r.size;
  loadmem_reqs.pop_front();
  return true;
}

void fesvr_channel_t::recv_loadmem_data(void* buf, size_t len) {
  std::copy(loadmem_data.begin(), loadmem_data.begin() + len, (char*)buf);
  loadmem_data.erase(loadmem_data.begin(), loadmem_data.begin() + len);
}

void fesvr_channel_t::tick() {
  bool ready;
  do {
    out.acquire();
    if ((ready = out.ready())) {
      if ((out[0] = !rdata.empty())) {
        out[1] = rdata.front();
	rdata.pop_front();
#ifdef __DEBUG__
        fprintf(stderr, "[fesvr_channel] read data: %llx\n", out[1]);
        fflush(stderr);
#endif
      }
      out.produce();
    }
    out.release();
  } while(!ready);
  bool valid;
  do {
    in.acquire();
    if ((valid = in.valid())) {
      fesvr_started = in[0] & 0x1;
      if ((fesvr_done = (in[0] >> 1) & 0x1)) {
        exitcode = in[1];
#ifdef __DEBUG__
        fprintf(stderr, "[fesvr_channel] exitcode: %lld\n", exitcode);
        fflush(stderr);
#endif
      }
      bool req_wr = ((in[0] >> 2) & 0x1);
      bool req_valid = ((in[0] >> 3) & 0x1);
      bool loadmem_valid = ((in[0] >> 4) & 0x1);
      if (req_valid) {
	size_t addr = in[2];
        mem_reqs.push_back(fesvr_mem_t(req_wr, addr));
	if (req_wr) {
	  uint64_t data = in[3];
	  wdata.push_back(data);
#ifdef __DEBUG__
          fprintf(stderr, "[fesvr_channel] write addr: %llx, data: %llx\n", addr, data);
          fflush(stderr);
#endif
        } else {
#ifdef __DEBUG__
          fprintf(stderr, "[fesvr_channel] read addr: %llx\n", addr);
          fflush(stderr);
#endif
        }
      }
      if (loadmem_valid) {
	size_t addr = in[4];
	size_t size = in[5];
	char* data = (char*)&in[6];
        loadmem_reqs.push_back(fesvr_loadmem_t(addr, size));
	loadmem_data.insert(loadmem_data.end(), data, data + size);
#ifdef __DEBUG__
        fprintf(stderr, "[fesvr_channel] loadmem addr:%llx, size: %lld\n", addr, size);
        fflush(stderr);
#endif
      }
      in.consume();
    }
    in.release();
  } while(!valid);
}

int main(int argc, char** argv) {
  fesvr_channel_t fesvr;
  midas_top_catapult_t midas_top(argc, argv, &fesvr);
  midas_top.init(argc, argv, false);
  midas_top.run(128);
  return midas_top.finish();
}
