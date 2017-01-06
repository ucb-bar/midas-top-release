#include "midas_top_catapult.h"

fesvr_channel_t::fesvr_channel_t():
  in("in"), out("out"), status("status")
{
  status.release();
  out.release();
  in.release();
}

fesvr_channel_t::~fesvr_channel_t()
{
}

bool fesvr_channel_t::done() {
  bool valid;
  status.acquire();
  if ((valid = status.valid())) {
    exitcode = (int)status[0];
    // fprintf(stderr, "[fesvr_channel] exitcode: %d\n", exitcode);
    // fflush(stderr);
    status.consume();
  }
  status.release();
  return valid;
}

bool fesvr_channel_t::data_available() {
  bool valid;
  out.acquire();
  if ((valid = out.valid())) {
    data = out[0];
    // fprintf(stderr, "[fesvr_channel] read: %x\n", data);
    // fflush(stderr);
    out.consume();
  }
  out.release();
  return valid;
}

void fesvr_channel_t::send_word(uint32_t word) {
  bool ready;
  do {
    in.acquire();
    if ((ready = in.ready())) {
      // fprintf(stderr, "[fesvr_channel] write: %x\n", word);
      // fflush(stderr);
      in[0] = word;
      in.produce();
    }
    in.release();
  } while(!ready);
}

int main(int argc, char** argv) {
  fesvr_channel_t fesvr;
  midas_top_catapult_t midas_top(argc, argv, &fesvr);
  midas_top.init(argc, argv, false);
  midas_top.run(128);
  return midas_top.finish();
}
