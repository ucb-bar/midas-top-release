#include "midas_tsi.h"

int midas_tsi_t::host_thread(void *arg)
{
  midas_tsi_t *tsi = static_cast<midas_tsi_t*>(arg);
  tsi->run();

  while (true)
    tsi->target->switch_to();

  return 0;
}

midas_tsi_t::midas_tsi_t(const std::vector<std::string>& args) : midas_fesvr_t(args)
{
  idle_counts = 10;
  for (auto& arg: args) {
    if (arg.find("+idle-counts=") == 0) {
      idle_counts = atoi(arg.c_str()+13);
    }
  }
  target = midas_context_t::current();
  host.init(host_thread, this);
}

midas_tsi_t::~midas_tsi_t()
{
}

void midas_tsi_t::tick()
{
  host.switch_to();
}

void midas_tsi_t::idle() {
  for (size_t i = 0 ; i < idle_counts ; i++)
    target->switch_to();
}

void midas_tsi_t::send_word(uint32_t word)
{
  out_data.push_back(word);
}

uint32_t midas_tsi_t::recv_word(void)
{
  uint32_t word = in_data.front();
  in_data.pop_front();
  return word;
}

bool midas_tsi_t::data_available(void)
{
  return !in_data.empty();
}

void midas_tsi_t::read(uint32_t* data, size_t len) {
  for (size_t i = 0 ; i < len ; i++) {
    while (out_data.empty()) target->switch_to();
    data[i] = out_data.front();
    out_data.pop_front();
  }
}

void midas_tsi_t::write(const uint32_t* data, size_t len) {
  in_data.insert(in_data.end(), data, data + len);
}
