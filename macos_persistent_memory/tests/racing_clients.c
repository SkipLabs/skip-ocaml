#include "../core.h"

#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

static int child_worker(const char* path, mach_vm_address_t addr,
                        size_t size, uint64_t marker) {
  int fd = pmem_prepare_file(path, size, O_CREAT);
  if (fd == -1) {
    perror("[child] pmem_prepare_file");
    return 2;
  }
  pmem_mapping mapping;
  if (pmem_map_file_flags(fd, size, addr, PROT_READ | PROT_WRITE,
                          MAP_SHARED, VM_FLAGS_FIXED, &mapping) != 0) {
    perror("[child] pmem_map_file_flags");
    close(fd);
    return 1;
  }
  uint64_t* ptr = (uint64_t*)mapping.addr;
  ptr[0] = marker;
  msync(ptr, sizeof(uint64_t), MS_SYNC);
  printf("[child %d] mapped at %p wrote 0x%llx\n", getpid(), ptr,
         (unsigned long long)marker);
  sleep(2);  // keep mapping alive to increase contention window
  pmem_unmap(&mapping);
  close(fd);
  return 0;
}

int main(void) {
  const char* path = "macos_persistent_memory/race.bin";
  const mach_vm_address_t addr = 0x333000000ULL;
  const size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
  const size_t size = page_size * 2;

  pid_t a = fork();
  if (a == -1) {
    perror("fork");
    return 1;
  }
  if (a == 0) {
    int rc = child_worker(path, addr, size, 0x1111111111111111ULL);
    _exit(rc);
  }

  pid_t b = fork();
  if (b == -1) {
    perror("fork");
    kill(a, SIGKILL);
    return 1;
  }
  if (b == 0) {
    int rc = child_worker(path, addr, size, 0x2222222222222222ULL);
    _exit(rc);
  }

  int status_a = 0, status_b = 0;
  waitpid(a, &status_a, 0);
  waitpid(b, &status_b, 0);

  printf("[parent] childA=%d childB=%d\n", WEXITSTATUS(status_a),
         WEXITSTATUS(status_b));
  if (WEXITSTATUS(status_a) == 0 && WEXITSTATUS(status_b) == 0) {
    printf("[parent] both children mapped successfully (separate VM spaces)\n");
    printf("[parent] contention must be handled by a higher-level coordinator\n");
  } else {
    printf("[parent] at least one child failed to map while the other held the"
           " range\n");
  }
  return 0;
}

