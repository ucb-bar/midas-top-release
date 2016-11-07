#include "simif_zynq.h"
#include "midas_top.h"

class midas_top_zynq_t:
  public simif_zynq_t,
  public midas_top_t
{
public:
  midas_top_zynq_t(int argc, char** argv):
    midas_top_t(argc, argv) { }
};

int main(int argc, char** argv) {
  midas_top_zynq_t midas_top(argc, argv);
  midas_top.init(argc, argv, false);
  midas_top.run(128);
  return midas_top.finish();
}
