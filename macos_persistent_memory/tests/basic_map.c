#include "../core.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

int main(void) {
  const char* path = "macos_persistent_memory/basic_map.bin";
  const size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
  const size_t map_size = page_size * 4;
  const mach_vm_address_t target = 0x330000000ULL;

  int fd = pmem_prepare_file(path, map_size, O_CREAT);
  if (fd == -1) {
    perror("pmem_prepare_file");
    return 1;
  }

  pmem_mapping mapping;
  if (pmem_map_file(fd, map_size, target, PROT_READ | PROT_WRITE,
                    MAP_SHARED, &mapping) != 0) {
    perror("pmem_map_file");
    close(fd);
    return 1;
  }

  uint64_t* ptr = (uint64_t*)mapping.addr;
  printf("Mapped at %p\n", ptr);
  printf("Initial value=0x%llx\n", (unsigned long long)ptr[0]);

  ptr[0] = 0x0123456789ABCDEFULL;
  msync(ptr, sizeof(uint64_t), MS_SYNC);
  printf("Updated value=0x%llx\n", (unsigned long long)ptr[0]);

  pmem_unmap(&mapping);
  close(fd);

  fd = open(path, O_RDONLY);
  uint64_t disk_value = 0;
  read(fd, &disk_value, sizeof(disk_value));
  close(fd);
  printf("Disk value=0x%llx\n", (unsigned long long)disk_value);
  return disk_value == 0x0123456789ABCDEFULL ? 0 : 1;
}

