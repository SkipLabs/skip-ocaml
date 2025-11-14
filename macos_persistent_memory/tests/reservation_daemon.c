#include "../core.h"

#include <errno.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int running = 1;

typedef struct {
  mach_vm_address_t addr;
  size_t size;
} reservation_t;

static reservation_t reservation;

static void handle_signal(int sig) {
  (void)sig;
  running = 0;
}

static void release_reservation(void) {
  if (reservation.addr != 0 && reservation.size != 0) {
    mach_vm_deallocate(mach_task_self(), reservation.addr, reservation.size);
    reservation.addr = 0;
    reservation.size = 0;
  }
}

int main(int argc, char** argv) {
  if (argc != 3) {
    fprintf(stderr, "usage: %s <hex_address> <size_bytes>\n", argv[0]);
    return 1;
  }

  reservation.addr = strtoull(argv[1], NULL, 0);
  reservation.size = strtoull(argv[2], NULL, 0);
  if (reservation.addr == 0 || reservation.size == 0) {
    fprintf(stderr, "invalid address or size\n");
    return 1;
  }

  kern_return_t kr = mach_vm_allocate(
      mach_task_self(), &reservation.addr, reservation.size,
      VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE);
  if (kr != KERN_SUCCESS) {
    fprintf(stderr, "mach_vm_allocate failed: %s\n",
            mach_error_string(kr));
    return 1;
  }

  printf("[daemon] reserved range [%p, %p)\n",
         (void*)reservation.addr,
         (void*)(reservation.addr + reservation.size));

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handle_signal;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  while (running) {
    pause();
  }

  printf("[daemon] releasing reservation\n");
  release_reservation();
  return 0;
}

