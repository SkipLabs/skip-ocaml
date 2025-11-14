#include "../core.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

typedef struct {
  uint64_t magic;
  uintptr_t image_base;
} pmem_header_t;

static int write_header(int fd, const pmem_header_t* header) {
  if (lseek(fd, 0, SEEK_SET) == -1) {
    return -1;
  }
  ssize_t written = write(fd, header, sizeof(*header));
  return (written == (ssize_t)sizeof(*header)) ? 0 : -1;
}

static int read_header(int fd, pmem_header_t* header) {
  if (lseek(fd, 0, SEEK_SET) == -1) {
    return -1;
  }
  ssize_t bytes = read(fd, header, sizeof(*header));
  return (bytes == (ssize_t)sizeof(*header)) ? 0 : -1;
}

int main(void) {
  const char* path = "macos_persistent_memory/image_base.bin";
  const size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
  const size_t size = page_size;
  const mach_vm_address_t target = 0x360000000ULL;
  const uint64_t MAGIC = 0x504D454D554CULL; /* "PMEMUL" */

  int fd = pmem_prepare_file(path, size, O_CREAT);
  if (fd == -1) {
    perror("pmem_prepare_file");
    return 1;
  }

  pmem_header_t header = {
      .magic = MAGIC,
      .image_base = pmem_current_image_base(),
  };

  if (write_header(fd, &header) != 0) {
    perror("write header");
    close(fd);
    return 1;
  }

  pmem_mapping mapping;
  if (pmem_map_file(fd, size, target, PROT_READ | PROT_WRITE, MAP_SHARED,
                    &mapping) != 0) {
    perror("pmem_map_file");
    close(fd);
    return 1;
  }

  memcpy(mapping.addr, &header, sizeof(header));
  msync(mapping.addr, sizeof(header), MS_SYNC);
  pmem_unmap(&mapping);
  close(fd);

  printf("First mapping succeeded with image base %p\n",
         (void*)header.image_base);

  fd = open(path, O_RDWR);
  if (fd == -1) {
    perror("open");
    return 1;
  }

  pmem_header_t loaded;
  if (read_header(fd, &loaded) != 0) {
    perror("read header");
    close(fd);
    return 1;
  }

  uintptr_t current_base = pmem_current_image_base();
  if (loaded.magic != MAGIC || loaded.image_base != current_base) {
    fprintf(stderr, "Unexpected header mismatch on second load\n");
    close(fd);
    return 1;
  }
  printf("Second load confirmed matching image base %p\n",
         (void*)loaded.image_base);

  /* Simulate another executable by tampering with the stored base. */
  loaded.image_base ^= 0x1000ULL;
  if (write_header(fd, &loaded) != 0) {
    perror("corrupt header");
    close(fd);
    return 1;
  }
  close(fd);

  fd = open(path, O_RDONLY);
  if (fd == -1) {
    perror("re-open");
    return 1;
  }
  if (read_header(fd, &loaded) != 0) {
    perror("read corrupted header");
    close(fd);
    return 1;
  }
  close(fd);

  if (loaded.image_base != current_base) {
    printf("Detected image base mismatch (file=%p current=%p) — refusing to map\n",
           (void*)loaded.image_base, (void*)current_base);
    return 0;
  }

  fprintf(stderr, "Image base unexpectedly matched after corruption\n");
  return 1;
}

