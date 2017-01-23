#ifndef __SWITCH_H
#define __SWITCH_H

#define __SWITCH__

#include "simpleswitchcore.h"
#include "simif.h"
#ifdef __SWITCH__
#include "channel.h"
#endif

class switch_t
{
public:
  switch_t(simif_t* sim);
  ~switch_t();

  void send();
  void recv();
  void tick();
  bool busy();

private:
  simif_t* sim;
  struct {
    struct {
      transport_value value;
      bool ready;
      bool fire() { return !value.is_empty && ready; }
    } in, out;
  } data;
#ifdef __SWITCH__
  channel_t in;
  channel_t out;

  void send_switch(transport_value& value);
  void recv_switch(transport_value& value);
#else
  simpleswitch* sw;
#endif
};

#endif // __SWITCH_H
