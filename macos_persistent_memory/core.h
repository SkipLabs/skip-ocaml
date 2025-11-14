#ifndef MACOS_PMEM_CORE_H
#define MACOS_PMEM_CORE_H

#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach/vm_statistics.h>

#ifndef __VM_FLAGS_T_DEFINED_CUSTOM__
typedef int vm_flags_t;
#define __VM_FLAGS_T_DEFINED_CUSTOM__ 1
#endif
#include <stddef.h>
#include <stdint.h>

typedef struct {
  void* addr;
  size_t size;
  mach_vm_address_t target;
} pmem_mapping;

int pmem_prepare_file(const char* path, size_t size, int extra_flags);
int pmem_map_file_flags(int fd, size_t size, mach_vm_address_t target, int prot,
                        int map_flags, vm_flags_t vm_flags,
                        pmem_mapping* out);
int pmem_map_file(int fd, size_t size, mach_vm_address_t target, int prot,
                  int map_flags, pmem_mapping* out);
int pmem_unmap(const pmem_mapping* mapping);

#endif /* MACOS_PMEM_CORE_H */

