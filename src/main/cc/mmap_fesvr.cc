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
      bool rdata_valid = out[0];
      if (rdata_valid) {
        uint64_t data = out[1];
        rdata.push_back(data);
#ifdef __DEBUG__
        fprintf(stderr, "[mmap_fesvr] read data: %llx\n", rdata.back());
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
      in[0] = started();
      if (done()) {
	in[0] |= (done() << 1);
        in[1] = exit_code();
#ifdef __DEBUG__
        fprintf(stderr, "[mmap_fesvr] exitcode: %d\n", in[1]);
        fflush(stderr);
#endif
      }
      if (!mem_reqs.empty()) {
	auto req = mem_reqs.front();
	in[0] |= (req.wr << 2);
	in[0] |= (0x1 << 3);
	in[2] = req.addr;
	mem_reqs.pop_front();
	if (req.wr) {
          in[3] = wdata.front();
	  wdata.pop_front();
#ifdef __DEBUG__
          fprintf(stderr, "[mmap_fesvr] write addr: %llx, data: %llx\n", in[2], in[3]);
          fflush(stderr);
#endif
        } else {
#ifdef __DEBUG__
          fprintf(stderr, "[mmap_fesvr] read addr: %llx\n", in[2]);
          fflush(stderr);
#endif
	}
      }
      if (!loadmem_reqs.empty()) {
        auto loadmem = loadmem_reqs.front();
	in[0] |= (0x1 << 4);
	in[4] = loadmem.addr;
	in[5] = loadmem.size;
        std::copy(loadmem_data.begin(), loadmem_data.begin() + loadmem.size, (char*)&in[6]);
	loadmem_data.erase(loadmem_data.begin(), loadmem_data.begin() + loadmem.size);
	loadmem_reqs.pop_front();
#ifdef __DEBUG__
        fprintf(stderr, "[mmap_fesvr] loadmem addr: %llx, size: %llu\n", in[4], in[5]);
        char* base = (char*)&in[6];
	for (size_t off = 0 ; off < loadmem.size ; off += sizeof(uint64_t)) {
          fprintf(stderr, "[mmap_fesvr] mem[%llx] <- %016llx\n", loadmem.addr + off, *(uint64_t*)(base + off));
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
