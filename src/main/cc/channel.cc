#include "channel.h"
#include <cassert>
#define BUF_SIZE 2048

channel_t::channel_t(const char* fname)
{
#ifndef _WIN32
  assert(file = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, BUF_SIZE, fname));
#else
  assert(file = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, fname));
#endif
  assert(channel = (char*)MapViewOfFile(file, FILE_MAP_ALL_ACCESS, 0, 0, BUF_SIZE));
}

channel_t::~channel_t() {
  UnmapViewOfFile((LPCVOID)channel);
  CloseHandle(file);
}
