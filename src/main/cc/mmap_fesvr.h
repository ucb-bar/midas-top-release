#ifndef __MMAP_FESVR_H
#define __MMAP_FESVR_H

#include "midas_fesvr.h"
#include "channel.h"

class mmap_fesvr_t : public midas_fesvr_t
{
 public:
  mmap_fesvr_t(const std::vector<std::string>& args);
  virtual ~mmap_fesvr_t();
  virtual void wait();

 private:
  channel_t in;
  channel_t out;
};

#endif // __MMAP_FESVR_H
