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
      out[0] = rdata.empty();
      if (!rdata.empty()) {
        out[1] = rdata.front();
	rdata.pop_front();
#ifdef __DEBUG__
	if (out[1] != 0) {
          fprintf(stderr, "[fesvr_channel] read data: %llx\n", out[1]);
          fflush(stderr);
	}
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
      fesvr_busy = in[0];
      fesvr_done = in[1];
      exitcode = in[2];
#ifdef __DEBUG__
      if (fesvr_done) {
        fprintf(stderr, "[fesvr_channel] exitcode: %d\n", exitcode);
        fflush(stderr);
      }
#endif
      bool mem_empty = in[3];
      bool loadmem_empty = in[4];
      if (!mem_empty) {
        bool wr = in[5];
	size_t addr = in[6];
	uint64_t data = in[7];
        mem_reqs.push_back(fesvr_mem_t(wr, addr));
        if (wr) wdata.push_back(data);
#ifdef __DEBUG__
	if (wr) {
          fprintf(stderr, "[fesvr_channel] write addr: %zx, data: %zx\n", addr, data);
        } else {
          // fprintf(stderr, "[fesvr_channel] read addr: %zx\n", addr);
        }
        fflush(stderr);
#endif
      }
      if (!loadmem_empty) {
	size_t addr = in[8];
	size_t size = in[9];
	char* data = (char*)&in[10];
        loadmem_reqs.push_back(fesvr_loadmem_t(addr, size));
	loadmem_data.insert(loadmem_data.end(), data, data + size);
#ifdef __DEBUG__
        // fprintf(stderr, "[fesvr_channel] loadmem addr:%zx, size: %zx\n", addr, size);
        // fflush(stderr);
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
  midas_top.run(2048);
  return midas_top.finish();
}
