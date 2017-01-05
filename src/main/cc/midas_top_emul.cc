#include "simif_emul.h"
#include "midas_top.h"
#include "midas_tsi.h"

class midas_top_emul_t:
  public simif_emul_t,
  public midas_top_t
{
public:
  midas_top_emul_t(int argc, char** argv):
    midas_top_t(argc, argv) { }
};

int main(int argc, char** argv) {
  midas_top_emul_t midas_top(argc, argv);
  serial_tsi_t serial(&midas_top, argc, argv);
  midas_top.add(&serial);
  midas_top.init(argc, argv);
  midas_top.run(128);
  return midas_top.finish();
}
