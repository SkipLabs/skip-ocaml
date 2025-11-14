#include "../core.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

static const mach_vm_address_t kTarget = 0x332000000ULL;

int main(void) {
  const char* path = "macos_persistent_memory/crash.bin";
  const size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
  const size_t map_size = page_size * 2;

  int fd = pmem_prepare_file(path, map_size, 0);
  if (fd == -1) {
    perror("pmem_prepare_file");
    return 1;
  }

  pmem_mapping mapping;
  if (pmem_map_file(fd, map_size, kTarget, PROT_READ,
                    MAP_SHARED, &mapping) != 0) {
    perror("pmem_map_file");
    close(fd);
    return 1;
  }

  uint64_t value = *(uint64_t*)mapping.addr;
  printf("[verifier] value=0x%llx at %p\n",
         (unsigned long long)value, mapping.addr);

  pmem_unmap(&mapping);
  close(fd);
  return value == 0xDEADBEEFCAFEBABEULL ? 0 : 1;
}

