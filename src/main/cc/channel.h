#ifndef __CHANNEL_H
#define __CHANNEL_H

#include <stdint.h>
#include <windows.h>

class channel_t {
public:
  channel_t(const char* fname);
  ~channel_t();
  inline void acquire() {
#ifndef _WIN32
    channel[0] = 1;
    channel[2] = 0;
    while (channel[1] == 1 && channel[2] == 0);
#else
    channel[1] = 1;
    channel[2] = 1;
    while (channel[0] == 1 && channel[2] == 1);
#endif
  }
  inline void release() {
#ifndef _WIN32
    channel[0] = 0;
#else
    channel[1] = 0;
#endif
  }
  inline void produce() { channel[3] = 1; }
  inline void consume() { channel[3] = 0; }
  inline bool ready() { return channel[3] == 0; }
  inline bool valid() { return channel[3] == 1; }
  inline uint64_t* data() { return (uint64_t*)(channel + 4); }
  inline uint64_t& operator[](int i) { return data()[i]; }
private:
  // Dekker's algorithm for sync
  // channel[0] -> fesvr
  // channel[1] -> sim
  // channel[2] -> turn
  // channel[3] -> flag
  // channel[4:] -> data
  volatile char* channel;
  HANDLE file;
};

#endif // __CHANNEL_H
