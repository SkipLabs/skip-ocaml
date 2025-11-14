# macOS Persistent Memory Experiments

This directory hosts small, isolated experiments that explore how to emulate
Skip’s Linux-style persistent heap on macOS. The goal is to keep each concern
separate from the main runtime so we can validate tricky behaviors before
attempting a full integration.

## Layout

- `core.c` / `core.h` – tiny helpers to reserve fixed virtual ranges via
  `mach_vm_allocate()` and map files with `MAP_FIXED`.
- `tests/`
  - `basic_map.c` – sanity check that a file can be mapped at a fixed address,
    mutated, and persisted.
  - `fork_share.c` – verifies child processes inherit the mapping and see each
    other’s writes.
  - `crash_writer.c` / `crash_verifier.c` – simulates a process dying while the
    mapping is active and confirms the next process can reuse the same address
    with data intact.
  - `reservation_daemon.c` – reserves a fixed virtual range and holds it until
    terminated (prototype for a future privileged helper).
  - `client_fixed_map.c` – maps a file at a user-supplied address; useful for
    manual tests while the daemon is running.
  - `racing_clients.c` – forks two children that both try to claim the same
    range without `VM_FLAGS_OVERWRITE`, highlighting that we need external
    coordination to prevent contention.
  - `paging_stress.c` – touches a large (configurable) mapped file to exercise
    page faults, eviction, and msync on macOS.
  - `sigbus_test.c` – intentionally invalidates a mapping (truncates the backing
    file) to observe whether macOS delivers `SIGBUS` like Linux.

## Building

These programs are intentionally standalone; build them with Clang directly
from the repo root:

```sh
clang macos_persistent_memory/core.c \
      macos_persistent_memory/tests/basic_map.c \
      -o macos_persistent_memory/tests/basic_map
```

Each test prints its expected behavior on success. Example invocations:

```sh
# Basic mapping
clang macos_persistent_memory/core.c \
      macos_persistent_memory/tests/basic_map.c \
      -o macos_persistent_memory/tests/basic_map
./macos_persistent_memory/tests/basic_map

# Reservation daemon + client (run in two terminals)
./macos_persistent_memory/tests/reservation_daemon 0x340000000 8192
./macos_persistent_memory/tests/client_fixed_map \
    macos_persistent_memory/reservation_test.bin \
    0x340000000 8192

# Racing clients (shows one child failing without coordination)
./macos_persistent_memory/tests/racing_clients
```

Use `SKIP_PALLOC_DEBUG=1` with the main runtime to mirror this logging style
during integration.

## Next Steps

Additional experiments to add here before touching the real runtime:

- Turn the reservation daemon into a true coordinator (hand out Mach memory
  entries instead of clients calling `mach_vm_allocate` themselves).
- Expand paging/SIGBUS stress coverage (e.g., multi-GB heaps, concurrent
  readers, simulated I/O errors).
- Simulate privileged helper + client handshake (Mach ports, leases, etc.).

