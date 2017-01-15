#ifndef __MMAP_FESVR_H
#define __MMAP_FESVR_H

#include "midas_fesvr.h"
#include "channel.h"

class mmap_fesvr_t : public midas_fesvr_t
{
 public:
  mmap_fesvr_t(const std::vector<std::string>& args);
  virtual ~mmap_fesvr_t();
  void wait();
 private:
  channel_t in;
  channel_t out;
  std::deque<uint32_t> in_data;
  std::deque<uint32_t> out_data;
  virtual void read(uint32_t* data, size_t len);
  virtual void write(const uint32_t* data, size_t len);
};

#endif // __MMAP_FESVR_H
