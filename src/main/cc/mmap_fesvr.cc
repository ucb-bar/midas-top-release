#include "mmap_fesvr.h"

mmap_fesvr_t::mmap_fesvr_t(const std::vector<std::string>& args):
  midas_fesvr_t(args), in("in"), out("out"), status("status")
{
  in.consume();
  out.consume();
  status.consume();
  in.release();
  out.release();
  status.release();
}

mmap_fesvr_t::~mmap_fesvr_t()
{
}

void mmap_fesvr_t::read(uint32_t* data, size_t len) {
  for (size_t i = 0 ; i < len ; i++) {
    bool valid;
    do {
      in.acquire();
      if (valid = in.valid()) {
        data[i] = in[0];
        // fprintf(stderr, "[mmap_fesvr] read: %x\n", data[i]);
        // fflush(stderr);
        in.consume();
      }
      in.release();
    } while(!valid);
  } 
}

void mmap_fesvr_t::write(const uint32_t* data, size_t len) {
  for (size_t i = 0 ; i < len ; i++) {
    bool ready;
    do {
      out.acquire();
      if ((ready = out.ready())) {
        // fprintf(stderr, "[mmap_fesvr] write: %x\n", data[i]);
        // fflush(stderr);
        out[0] = data[i];
	out.produce();
      }
      out.release();
    } while(!ready);
  }
}

void mmap_fesvr_t::send_exitcode(int exitcode) {
  bool ready;
  do {
    status.acquire();
    if (ready = status.ready()) {
      // fprintf(stderr, "[mmap_fesvr] exitcode: %d\n", exitcode);
      // fflush(stderr);
      status[0] = exitcode;
      status.produce();
    }
    status.release();
  } while(!ready);
}

int main(int argc, char** argv) {
  std::vector<std::string> args(argv + 1, argv + argc);
  mmap_fesvr_t fesvr(args);
  fesvr.send_exitcode(fesvr.run());
  return fesvr.exit_code();
}
