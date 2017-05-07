#include "mmap_fesvr.h"

// #define __DEBUG__

mmap_fesvr_t::mmap_fesvr_t(const std::vector<std::string>& args):
  midas_fesvr_t(args), in("in"), out("out")
{
  in.consume();
  out.consume();
  in.release();
  out.release();
}

mmap_fesvr_t::~mmap_fesvr_t()
{
}

void mmap_fesvr_t::wait() {
  bool valid;
  do {
    out.acquire();
    if ((valid = out.valid())) {
      bool empty = out[0];
      if (!empty) {
        uint64_t data = out[1];
        out_data.push_back(data);
#ifdef __DEBUG__
        fprintf(stderr, "[mmap_fesvr] output data: %llx\n", out_data.back());
        fflush(stderr);
#endif
      }
      out.consume();
    }
    out.release();
  } while(!valid);

  bool ready;
  do {
    in.acquire();
    if ((ready = in.ready())) {
      in[0] = busy();
      in[1] = done();
      in[2] = exit_code();
      in[3] = in_data.empty();
      in[4] = loadmem_reqs.empty();
#ifdef __DEBUG__
      if (in[1]) {
        fprintf(stderr, "[mmap_fesvr] exitcode: %d\n", in[2]);
        fflush(stderr);
      }
#endif
      if (!in_data.empty()) {
	in[5] = in_data.front();
	in_data.pop_front();
#ifdef __DEBUG__
        fprintf(stderr, "[mmap_fesvr] input data: %llx\n", in[5]);
        fflush(stderr);
#endif
      }
      if (!loadmem_reqs.empty()) {
        auto loadmem = loadmem_reqs.front();
	in[6] = loadmem.addr;
	in[7] = loadmem.size;
        std::copy(loadmem_data.begin(), loadmem_data.begin() + loadmem.size, (char*)&in[8]);
	loadmem_data.erase(loadmem_data.begin(), loadmem_data.begin() + loadmem.size);
	loadmem_reqs.pop_front();
#ifdef __DEBUG__
        // fprintf(stderr, "[mmap_fesvr] loadmem addr: %llx, size: %llu\n", in[6], in[7]);
        char* base = (char*)&in[8];
	for (size_t off = 0 ; off < loadmem.size ; off += sizeof(uint64_t)) {
          // fprintf(stderr, "[mmap_fesvr] mem[%llx] <- %016llx\n", loadmem.addr + off, *(uint64_t*)(base + off));
	}
        fflush(stderr);
#endif
      }
      in.produce();
    }
    in.release();
  } while(!ready);
}

int main(int argc, char** argv) {
  std::vector<std::string> args(argv + 1, argv + argc);
  mmap_fesvr_t fesvr(args);
  int exitcode = fesvr.run();
  fesvr.wait(); // to send exitcode
  return exitcode;
}
