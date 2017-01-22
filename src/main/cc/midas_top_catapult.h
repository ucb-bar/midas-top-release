#include "simif_catapult.h"
#include "midas_top.h"
#include "fesvr_proxy.h"
#include "channel.h"

class fesvr_channel_t: public fesvr_proxy_t
{
 public:
   fesvr_channel_t();
   ~fesvr_channel_t();

   virtual void tick();
   virtual bool started() { return fesvr_started; }
   virtual bool busy() { return !mem_reqs.empty(); }
   virtual bool done() { return fesvr_done; }
   virtual int exit_code() { return exitcode; }

   virtual bool recv_mem_req(fesvr_mem_t& req);
   virtual uint64_t recv_mem_wdata();
   virtual void send_mem_rdata(uint64_t);

   virtual bool recv_loadmem_req(fesvr_loadmem_t& loadmem);
   virtual void recv_loadmem_data(void* buf, size_t len);

 private:
   channel_t in;
   channel_t out;
   std::deque<fesvr_mem_t> mem_reqs;
   std::deque<uint64_t> wdata;
   std::deque<uint64_t> rdata;
   std::deque<fesvr_loadmem_t> loadmem_reqs;
   std::deque<char> loadmem_data;
   bool fesvr_started;
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
