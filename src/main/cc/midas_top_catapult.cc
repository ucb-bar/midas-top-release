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

void fesvr_channel_t::send_word(uint32_t word) {
  out_data.push_back(word);
}

uint32_t fesvr_channel_t::recv_word() {
  uint32_t word = in_data.front();
  in_data.pop_front();
  return word;
}

bool fesvr_channel_t::data_available() { return !in_data.empty(); }

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
  bool outdata_empty, indata_empty, loadmem_empty;

  do {
    bool ready;
    do {
      out.acquire();
      if ((ready = out.ready())) {
        out[0] = (outdata_empty = out_data.empty());
        if (!out_data.empty()) {
          out[1] = out_data.front();
          out_data.pop_front();
#ifdef __DEBUG__
          fprintf(stderr, "[fesvr_channel] output data: %llx\n", out[1]);
          fflush(stderr);
#endif
        }
        out.produce();
      }
      out.release();
    } while(!ready);
  } while(!outdata_empty);
  
  do {
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
        if (!(indata_empty = in[3])) {
          uint64_t data = in[5];
          in_data.push_back(data);
#ifdef __DEBUG__
          fprintf(stderr, "[fesvr_channel] input data: %zx\n", data);
          fflush(stderr);
#endif
        }
        if (!(loadmem_empty = in[4])) {
          size_t addr = in[6];
          size_t size = in[7];
          char* data = (char*)&in[8];
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
  } while(!indata_empty || !loadmem_empty);
}

#ifndef NO_MAIN
int main(int argc, char** argv) {
  fesvr_channel_t fesvr;
  midas_top_catapult_t midas_top(argc, argv, &fesvr);
  midas_top.init(argc, argv, false);
  midas_top.run(2048);
  return midas_top.finish();
}
#endif
