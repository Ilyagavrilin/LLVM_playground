#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

uint64_t fact(uint64_t arg);

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: 1 argument - factorial len\n");
    return 1;
  }
  uint64_t arg = atoi(argv[1]);
  if (errno == 0) {
    printf("Fact(%lu) = %lu\n", arg, fact(arg));
  } else {
    printf("Usage: 1 argument - factorial len\n");
    return 1;
  }
  return 0;
}