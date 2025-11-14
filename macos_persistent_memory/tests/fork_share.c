#include "../core.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

static const mach_vm_address_t kTarget = 0x331000000ULL;

typedef struct {
  uintptr_t image_base;
  uint64_t value;
} shared_record_t;

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

  shared_record_t* record = (shared_record_t*)mapping.addr;
  record->image_base = pmem_current_image_base();
  record->value = 0xAAAAAAAAAAAAAAAAULL;
  msync(record, sizeof(*record), MS_SYNC);

  pid_t pid = fork();
  if (pid == -1) {
    perror("fork");
    pmem_unmap(&mapping);
    close(fd);
    return 1;
  }

  if (pid == 0) {
    printf("[child] initial=0x%llx\n", (unsigned long long)record->value);
    record->value = 0xBBBBBBBBBBBBBBBBULL;
    msync(&record->value, sizeof(uint64_t), MS_SYNC);
    printf("[child] wrote=0x%llx\n", (unsigned long long)record->value);
    _exit(0);
  }

  int status = 0;
  waitpid(pid, &status, 0);
  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    fprintf(stderr, "child failed\n");
    return 1;
  }

  printf("[parent] after child=0x%llx\n",
         (unsigned long long)record->value);
  record->value = 0xCCCCCCCCCCCCCCCCULL;
  msync(&record->value, sizeof(uint64_t), MS_SYNC);

  pmem_unmap(&mapping);
  close(fd);

  fd = open(path, O_RDONLY);
  shared_record_t disk_record = {0};
  read(fd, &disk_record, sizeof(disk_record));
  close(fd);
  printf("[parent] disk value=0x%llx\n",
         (unsigned long long)disk_record.value);
  if (disk_record.value != 0xCCCCCCCCCCCCCCCCULL) {
    return 1;
  }

  uintptr_t current_base = pmem_current_image_base();
  if (disk_record.image_base != current_base) {
    fprintf(stderr, "Unexpected image-base mismatch after fork\n");
    return 1;
  }

  /* Tamper with the stored base to emulate a different executable trying to
     reuse the heap. */
  fd = open(path, O_RDWR);
  if (fd == -1) {
    perror("open for tamper");
    return 1;
  }
  disk_record.image_base ^= 0x4000ULL;
  if (lseek(fd, 0, SEEK_SET) == -1 ||
      write(fd, &disk_record, sizeof(disk_record)) !=
          (ssize_t)sizeof(disk_record)) {
    perror("tamper write");
    close(fd);
    return 1;
  }
  close(fd);

  printf("[parent] stored image base tampered (file=%p current=%p)\n",
         (void*)disk_record.image_base, (void*)current_base);
  printf("[parent] a fresh executable should now refuse to map this heap\n");
  return 0;
}

