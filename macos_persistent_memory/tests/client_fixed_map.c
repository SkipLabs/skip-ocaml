#include "../core.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv) {
  if (argc != 4) {
    fprintf(stderr, "usage: %s <file> <hex_address> <size_bytes>\n", argv[0]);
    return 1;
  }

  const char* path = argv[1];
  mach_vm_address_t addr = strtoull(argv[2], NULL, 0);
  size_t size = strtoull(argv[3], NULL, 0);

  int fd = pmem_prepare_file(path, size, O_CREAT);
  if (fd == -1) {
    perror("pmem_prepare_file");
    return 1;
  }

  pmem_mapping mapping;
  if (pmem_map_file(fd, size, addr, PROT_READ | PROT_WRITE,
                    MAP_SHARED, &mapping) != 0) {
    perror("pmem_map_file");
    close(fd);
    return 1;
  }

  uint64_t* ptr = (uint64_t*)mapping.addr;
  printf("[client] mapped at %p value=0x%llx\n", ptr,
         (unsigned long long)ptr[0]);
  ptr[0] = 0xF00DF00DF00DF00DULL;
  msync(ptr, sizeof(uint64_t), MS_SYNC);
  printf("[client] wrote value=0x%llx\n", (unsigned long long)ptr[0]);

  pmem_unmap(&mapping);
  close(fd);
  return 0;
}

