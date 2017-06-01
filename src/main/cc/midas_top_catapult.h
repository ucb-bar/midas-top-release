#include "simif_catapult.h"
#include "midas_top.h"
#include "fesvr/fesvr_proxy.h"
#include "fesvr/channel.h"

class fesvr_channel_t: public fesvr_proxy_t
{
 public:
   fesvr_channel_t();
   ~fesvr_channel_t();

   virtual void tick();
   virtual bool busy() { return fesvr_busy; }
   virtual bool done() { return fesvr_done; }
   virtual int exit_code() { return exitcode; }

   virtual bool data_available();
   virtual void send_word(uint32_t word);
   virtual uint32_t recv_word();

   virtual bool recv_loadmem_req(fesvr_loadmem_t& loadmem);
   virtual void recv_loadmem_data(void* buf, size_t len);

 private:
   channel_t in;
   channel_t out;
   std::deque<uint32_t> in_data;
   std::deque<uint32_t> out_data;
   std::deque<fesvr_loadmem_t> loadmem_reqs;
   std::deque<char> loadmem_data;
   bool fesvr_busy;
   bool fesvr_done;
   int exitcode;
};

class midas_top_catapult_t:
  public simif_catapult_t,
  public midas_top_t
{
public:
  midas_top_catapult_t(int argc, char** argv, fesvr_proxy_t* fesvr):
    midas_top_t(argc, argv, fesvr) { }
};
