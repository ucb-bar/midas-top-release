//See LICENSE for license details.

#include "simif_emul.h"
#include "rocketchip.h"
#include "fesvr/midas_tsi.h"

class midas_top_emul_t:
  public simif_emul_t,
  public rocketchip_t
{
public:
  midas_top_emul_t(int argc, char** argv, fesvr_proxy_t* fesvr):
    rocketchip_t(argc, argv, fesvr) { }
};

int main(int argc, char** argv) {
  midas_tsi_t tsi(std::vector<std::string>(argv + 1, argv + argc));
  midas_top_emul_t midas_top(argc, argv, &tsi);
  midas_top.init(argc, argv);
  midas_top.run(128);
  return midas_top.finish();
}
