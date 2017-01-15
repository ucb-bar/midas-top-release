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

void fesvr_channel_t::tick() {
  bool ready;
  do {
    out.acquire();
    if ((ready = out.ready())) {
#ifdef __DEBUG__
      if (!out_data.empty()) {
        fprintf(stderr, "[fesvr_channel] write: %x\n", out_data.front());
        fflush(stderr);
      }
#endif
      out[1] = out_data.empty();
      if (!out_data.empty()) {
        out[0] = out_data.front();
	out_data.pop_front();
      }
      out.produce();
    }
    out.release();
  } while(!ready);

  bool valid;
  do {
    in.acquire();
    if ((valid = in.valid())) {
      uint32_t data = in[0];
      bool empty = in[1];
      fesvr_done = in[2];
      exitcode = in[3];
      in.consume();
      if (!empty) in_data.push_back(data);
#ifdef __DEBUG__
      if (!in_data.empty()) {
        fprintf(stderr, "[fesvr_channel] read: %x\n", in_data.front());
        fflush(stderr);
      }
      if (fesvr_done) {
        fprintf(stderr, "[fesvr_channel] exitcode: %d\n", exitcode);
        fflush(stderr);
      }
#endif
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
