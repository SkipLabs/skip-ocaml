#include "../core.h"

#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

static const mach_vm_address_t kTarget = 0x332000000ULL;

int main(void) {
  const char* path = "macos_persistent_memory/crash.bin";
  const size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
  const size_t map_size = page_size * 2;

  int fd = pmem_prepare_file(path, map_size, O_CREAT);
  if (fd == -1) {
    perror("pmem_prepare_file");
    return 1;
  }

  pmem_mapping mapping;
  if (pmem_map_file(fd, map_size, kTarget, PROT_READ | PROT_WRITE,
                    MAP_SHARED, &mapping) != 0) {
    perror("pmem_map_file");
    close(fd);
    return 1;
  }

  uint64_t* ptr = (uint64_t*)mapping.addr;
  ptr[0] = 0xDEADBEEFCAFEBABEULL;
  msync(ptr, sizeof(uint64_t), MS_SYNC);
  printf("[crash-writer] wrote 0x%llx and will abort\n",
         (unsigned long long)ptr[0]);

  raise(SIGKILL);
  return 0;
}

