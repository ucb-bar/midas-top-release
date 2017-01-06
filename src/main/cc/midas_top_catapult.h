#include "simif_catapult.h"
#include "midas_top.h"
#include "fesvr_proxy.h"
#include "channel.h"

class fesvr_channel_t: public fesvr_proxy_t
{
 public:
   fesvr_channel_t();
   ~fesvr_channel_t();
   virtual bool done();
   virtual bool data_available();
   virtual void send_word(uint32_t word);
   virtual void tick() { }
   virtual uint32_t recv_word() { return data; }
   virtual int exit_code() { return exitcode; }
 private:
   channel_t in;
   channel_t out;
   channel_t status;
   int exitcode;
   uint32_t data;
};

class midas_top_catapult_t:
  public simif_catapult_t,
  public midas_top_t
{
public:
  midas_top_catapult_t(int argc, char** argv, fesvr_proxy_t* fesvr):
    midas_top_t(argc, argv, fesvr) { }
};
