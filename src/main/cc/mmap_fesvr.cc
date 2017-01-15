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

void mmap_fesvr_t::read(uint32_t* data, size_t len) {
  for (size_t i = 0 ; i < len ; i++) {
    while(out_data.empty()) wait();
    data[i] = out_data.front();
    out_data.pop_front();
  } 
}

void mmap_fesvr_t::write(const uint32_t* data, size_t len) {
  in_data.insert(in_data.end(), data, data + len);
}

void mmap_fesvr_t::wait() {
  bool valid;
  do {
    out.acquire();
    if ((valid = out.valid())) {
      uint32_t data = out[0];
      bool empty = out[1];
      if (!empty) out_data.push_back(data);
      out.consume();
#ifdef __DEBUG__
      if (!out_data.empty()) {
        fprintf(stderr, "[mmap_fesvr] read: %x\n", out_data.back());
        fflush(stderr);
      }
#endif
    }
    out.release();
  } while(!valid);

  bool ready;
  do {
    in.acquire();
    if ((ready = in.ready())) {
#ifdef __DEBUG__
      if (!in_data.empty()) {
        fprintf(stderr, "[mmap_fesvr] write: %x\n", in_data.front());
        fflush(stderr);
      }
      if (done()) {
        fprintf(stderr, "[mmap_fesvr] exitcode: %d\n", exit_code());
        fflush(stderr);
      }
#endif
      in[1] = in_data.empty();
      if (!in_data.empty()) {
        in[0] = in_data.front();
	in_data.pop_front();
      }
      in[2] = done();
      in[3] = exit_code();
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
