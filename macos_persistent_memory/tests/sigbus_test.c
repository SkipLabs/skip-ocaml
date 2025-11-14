#include "../core.h"

#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

static void sigbus_handler(int sig, siginfo_t* info, void* ctx) {
  (void)sig;
  (void)ctx;
  (void)info;
  const char msg[] = "[sigbus] caught SIGBUS while touching truncated mapping\n";
  write(STDOUT_FILENO, msg, sizeof(msg) - 1);
  _exit(0);
}

int main(void) {
  const size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
  const size_t map_size = page_size * 2;
  const char* path = "macos_persistent_memory/sigbus.bin";
  const mach_vm_address_t addr = 0x360000000ULL;

  int fd = pmem_prepare_file(path, map_size, O_CREAT);
  if (fd == -1) {
    perror("pmem_prepare_file");
    return 1;
  }

  pmem_mapping mapping;
  if (pmem_map_file(fd, map_size, addr, PROT_READ | PROT_WRITE,
                    MAP_SHARED, &mapping) != 0) {
    perror("pmem_map_file");
    close(fd);
    return 1;
  }

  struct sigaction sa;
  sa.sa_sigaction = sigbus_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;
  sigaction(SIGBUS, &sa, NULL);

  printf("[sigbus] mapping created at %p (%zu bytes)\n",
         mapping.addr, map_size);
  fflush(stdout);

  // Shrink file to trigger SIGBUS on next access beyond new EOF
  if (ftruncate(fd, page_size / 2) != 0) {
    perror("ftruncate");
    return 1;
  }
  printf("[sigbus] truncated backing file to %zu bytes\n", page_size / 2);
  fflush(stdout);

  // Touch second page; should trigger SIGBUS.
  volatile uint8_t value = *((uint8_t*)mapping.addr + page_size + 16);
  printf("[sigbus] unexpected read success value=%u\n", value);

  pmem_unmap(&mapping);
  close(fd);
  return 2;
}

