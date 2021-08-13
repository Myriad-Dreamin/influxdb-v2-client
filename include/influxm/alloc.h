//
// Created by Myriad-Dreamin on 2021/8/13.
//

#ifndef INFLUXDBM_ALLOC_H
#define INFLUXDBM_ALLOC_H

#include <cstring>
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

#endif // INFLUXDBM_ALLOC_H
