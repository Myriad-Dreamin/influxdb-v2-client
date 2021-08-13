//
// Created by Myriad-Dreamin on 2021/8/13.
//

#ifndef INFLUXDBM_ALLOC_H
#define INFLUXDBM_ALLOC_H

#include <cstring>
#include <string>

#define macroAllocBuffer(name, size)                                           \
  char name[4096];                                                             \
  const int size = 4096
#define macroMemoryCopyN(target, src, src_size, current_size, guard_size)      \
  do {                                                                         \
    if ((src_size) + (current_size) > (guard_size))                            \
      return -1;                                                               \
    memcpy(target + (current_size), src, (src_size));                          \
    current_size += (src_size);                                                \
  } while (0)
#define macroMemoryPutConst(target, src, current_size, guard_size)             \
  macroMemoryCopyN(target, src, (sizeof((src)) - 1), current_size, guard_size)
#define macroMemoryPutStdStr(target, src, current_size, guard_size)            \
  macroMemoryCopyN(target, (&src[0]), src.size(), current_size, guard_size)
#define macroMemoryPutC(target, ch, current_size, guard_size)                  \
  do {                                                                         \
    if (current_size + 1 > guard_size)                                         \
      return -1;                                                               \
    target[current_size] = ch;                                                 \
    current_size++;                                                            \
  } while (0)
#define macroFreeBuffer(name, size) (void *)(0)
#define macroConstStrCmpN(target, prefix) \
  strncmp(target, prefix, sizeof(prefix)-1)

namespace influx_client { // NOLINT(modernize-concat-nested-namespaces)
namespace detail {
#if __cplusplus < 201700L
using string_view = const std::string &;
using to_string_view = std::string;
#else
#include <string_view>
using string_view = std::string_view;
using to_string_view = std::string_view;
#endif
} // namespace detail
} // namespace influx_client

#ifndef influxdb_if_inline
#define influxdb_if_inline inline
#endif

#ifndef influx_http_recv
#define influx_http_recv recv
#endif

#ifndef influx_http_writev
#define influx_http_writev writev
#endif

#endif // INFLUXDBM_ALLOC_H
