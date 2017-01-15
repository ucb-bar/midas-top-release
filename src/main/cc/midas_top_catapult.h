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
   virtual void send_word(uint32_t word) {
     out_data.push_back(word);
   }
   virtual uint32_t recv_word() {
     uint32_t word = in_data.front();
     in_data.pop_front();
     return word;
   }
   virtual bool data_available() { return !in_data.empty(); }
   virtual bool done() { return fesvr_done; }
   virtual int exit_code() { return exitcode; }
 private:
   channel_t in;
   channel_t out;
   std::deque<uint32_t> in_data;
   std::deque<uint32_t> out_data;
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
