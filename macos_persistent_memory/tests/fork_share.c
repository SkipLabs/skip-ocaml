#include "../core.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

static const mach_vm_address_t kTarget = 0x331000000ULL;

int main(void) {
  const char* path = "macos_persistent_memory/fork_share.bin";
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
  ptr[0] = 0xAAAAAAAAAAAAAAAAULL;
  msync(ptr, sizeof(uint64_t), MS_SYNC);

  pid_t pid = fork();
  if (pid == -1) {
    perror("fork");
    pmem_unmap(&mapping);
    close(fd);
    return 1;
  }

  if (pid == 0) {
    printf("[child] initial=0x%llx\n", (unsigned long long)ptr[0]);
    ptr[0] = 0xBBBBBBBBBBBBBBBBULL;
    msync(ptr, sizeof(uint64_t), MS_SYNC);
    printf("[child] wrote=0x%llx\n", (unsigned long long)ptr[0]);
    _exit(0);
  }

  int status = 0;
  waitpid(pid, &status, 0);
  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    fprintf(stderr, "child failed\n");
    return 1;
  }

  printf("[parent] after child=0x%llx\n", (unsigned long long)ptr[0]);
  ptr[0] = 0xCCCCCCCCCCCCCCCCULL;
  msync(ptr, sizeof(uint64_t), MS_SYNC);

  pmem_unmap(&mapping);
  close(fd);
  fd = open(path, O_RDONLY);
  uint64_t disk_value = 0;
  read(fd, &disk_value, sizeof(disk_value));
  close(fd);
  printf("[parent] disk value=0x%llx\n", (unsigned long long)disk_value);
  return disk_value == 0xCCCCCCCCCCCCCCCCULL ? 0 : 1;
}

