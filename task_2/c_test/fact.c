#include <stdint.h>
#include <stdlib.h>

uint64_t fact(uint64_t arg) {
  uint64_t res = 0;
  if (arg < 2) {
    res = 1;
  } else {
    uint64_t next = fact(arg - 1);
    res = arg * next;
  }
  return res;
}
