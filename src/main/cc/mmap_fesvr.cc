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
        rdata.push_back(data);
#ifdef __DEBUG__
	if (rdata.back() != 0L) {
          fprintf(stderr, "[mmap_fesvr] read data: %llx\n", rdata.back());
          fflush(stderr);
	}
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
      in[1] = done();
      in[2] = exit_code();
      in[3] = mem_reqs.empty();
      in[4] = loadmem_reqs.empty();
#ifdef __DEBUG__
      if (in[1]) {
        fprintf(stderr, "[mmap_fesvr] exitcode: %d\n", in[2]);
        fflush(stderr);
      }
#endif
      if (!mem_reqs.empty()) {
	auto req = mem_reqs.front();
	in[5] = req.wr;
	in[6] = req.addr;
	if (req.wr) {
          in[7] = wdata.front();
	  wdata.pop_front();
	}
	mem_reqs.pop_front();
#ifdef __DEBUG__
	if (in[5]) {
          fprintf(stderr, "[mmap_fesvr] write addr: %llx, data: %llx\n", in[6], in[7]);
	} else {
          // fprintf(stderr, "[mmap_fesvr] read addr: %llx\n", in[6]);
	}
        fflush(stderr);
#endif
      }
      if (!loadmem_reqs.empty()) {
        auto loadmem = loadmem_reqs.front();
	in[8] = loadmem.addr;
	in[9] = loadmem.size;
        std::copy(loadmem_data.begin(), loadmem_data.begin() + loadmem.size, (char*)&in[10]);
	loadmem_data.erase(loadmem_data.begin(), loadmem_data.begin() + loadmem.size);
	loadmem_reqs.pop_front();
#ifdef __DEBUG__
        // fprintf(stderr, "[mmap_fesvr] loadmem addr: %llx, size: %llu\n", in[8], in[9]);
        char* base = (char*)&in[10];
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
