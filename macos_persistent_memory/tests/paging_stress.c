#include "../core.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

static double now_sec(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main(int argc, char** argv) {
  const size_t default_mb = 512;  // 512 MB
  size_t total_mb = default_mb;
  if (argc >= 2) {
    total_mb = strtoull(argv[1], NULL, 0);
  }
  const size_t total_bytes = total_mb * 1024 * 1024;
  const size_t stride = 4 * 1024 * 1024;  // touch every 4 MB
  const char* path = "macos_persistent_memory/paging.bin";
  const mach_vm_address_t addr = 0x350000000ULL;

  printf("[paging] allocating %zu MB (%zu bytes)\n",
         total_mb, total_bytes);

  int fd = pmem_prepare_file(path, total_bytes, O_CREAT);
  if (fd == -1) {
    perror("pmem_prepare_file");
    return 1;
  }

  pmem_mapping mapping;
  if (pmem_map_file(fd, total_bytes, addr, PROT_READ | PROT_WRITE,
                    MAP_SHARED, &mapping) != 0) {
    perror("pmem_map_file");
    close(fd);
    return 1;
  }

  uint64_t* base = (uint64_t*)mapping.addr;
  size_t steps = total_bytes / stride;
  double start = now_sec();
  for (size_t i = 0; i < steps; i++) {
    size_t offset = (i * stride) / sizeof(uint64_t);
    base[offset] = 0x1234567800000000ULL | i;
  }
  double mid = now_sec();
  msync(base, total_bytes, MS_SYNC);
  double end = now_sec();

  printf("[paging] touched %zu steps stride=%zu MB: fault time=%.2fs sync time=%.2fs\n",
         steps, stride / (1024 * 1024), mid - start, end - mid);

  pmem_unmap(&mapping);
  close(fd);
  return 0;
}

