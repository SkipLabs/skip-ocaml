#include "core.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#ifdef __APPLE__
extern void* __dso_handle;
#endif

uintptr_t pmem_current_image_base(void) {
#ifdef __APPLE__
  return (uintptr_t)__dso_handle;
#else
  return 0;
#endif
}

int pmem_prepare_file(const char* path, size_t size, int extra_flags) {
  int flags = O_RDWR | extra_flags;
  int fd = open(path, flags, 0600);
  if (fd == -1) {
    return -1;
  }
  if (size > 0) {
    if (ftruncate(fd, (off_t)size) != 0) {
      int err = errno;
      close(fd);
      errno = err;
      return -1;
    }
  }
  return fd;
}

int pmem_map_file_flags(int fd, size_t size, mach_vm_address_t target, int prot,
                        int map_flags, vm_flags_t vm_flags,
                        pmem_mapping* out) {
  if (out == NULL) {
    errno = EINVAL;
    return -1;
  }
  mach_vm_address_t reserve_addr = target;
  kern_return_t kr =
      mach_vm_allocate(mach_task_self(), &reserve_addr, size, vm_flags);
  if (kr != KERN_SUCCESS) {
    errno = EINVAL;
    return -1;
  }

  void* mapped =
      mmap((void*)target, size, prot, map_flags | MAP_FIXED, fd, 0);
  if (mapped == MAP_FAILED) {
    mach_vm_deallocate(mach_task_self(), target, size);
    return -1;
  }

  out->addr = mapped;
  out->size = size;
  out->target = target;
  return 0;
}

int pmem_map_file(int fd, size_t size, mach_vm_address_t target, int prot,
                  int map_flags, pmem_mapping* out) {
  return pmem_map_file_flags(fd, size, target, prot, map_flags,
                             VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE, out);
}

int pmem_unmap(const pmem_mapping* mapping) {
  if (mapping == NULL || mapping->addr == NULL || mapping->size == 0) {
    errno = EINVAL;
    return -1;
  }
  munmap(mapping->addr, mapping->size);
  mach_vm_deallocate(mach_task_self(), mapping->target, mapping->size);
  return 0;
}

