# Reactive: An Incremental Computation Framework for OCaml

Reactive is an OCaml library for building high-performance, incremental and reactive data pipelines. It enables you to cache computations across runs, re-execute only what’s necessary when inputs change, and avoid recomputing everything from scratch.

This library is built on top of the Skip runtime developed at SkipLabs. It brings reactive programming principles to OCaml by providing an API to define persistent computations over immutable collections of data.

---

## Key Concepts

- **Reactive collections** (`'a t`) represent immutable mappings from keys (strings) to arrays of values.
- **Trackers** allow the system to monitor external file dependencies.
- **Maps** define transformations between collections, forming a DAG (Directed Acyclic Graph) of computation.
- **Persistent cache** ensures efficient re-execution, storing artifacts across program runs.
- **Isolation and safety**: all reactive values are deeply immutable; unsafe operations are disallowed.

---

## Installation

This is a prototype and not yet released via opam. To use it:

```sh
git clone https://github.com/SkipLabs/skip-ocaml.git
cd skip-ocaml
make
```

## Building and Linking with Reactive

Reactive includes a custom runtime and linking requirements that are necessary to support persistent memoization of OCaml computations. You must take care to link your OCaml binary with the proper options for code-pointer stability and immutability enforcement.

### Build Instructions

After cloning the repository and running `make`, you can link your own OCaml program as follows (Linux example shown — see the note below for macOS):

```sh
ocamlopt -g -o my_program \
  -I build \
  unix.cmxa \
  reactive.cmxa \
  my_program.ml \
  build/libskip_reactive.a \
  -cclib -lstdc++ \
  -I +unix \
  -ccopt -no-pie \
  -ccopt -Wl,-Ttext=0x8000000
```

### Required Linker Options (Linux)

* `-ccopt -no-pie`  
  Disables position-independent execution so that function pointers remain stable across runs.

* `-ccopt -Wl,-Ttext=0x8000000`  
  Places all OCaml code at a fixed virtual address, which is critical for cache stability.

* `-cclib -lstdc++`
  Required because the runtime includes C++ components.

* `build/libskip_reactive.a`
  Static library bundling all the necessary C/C++ components, the Skip runtime, and memory/packing utilities.

* `reactive.cmxa`
  The OCaml-side interface to the Reactive system. Includes the main entry point and API.

### Notes

* macOS builds are fully supported. The provided Makefile handles Mach-based fixed mappings automatically, so you can omit the Linux-only linker flags (`-no-pie`, `-Wl,-Ttext=…`) when linking your own binaries on macOS.
* Your OCaml program must be compiled with `ocamlopt`. Bytecode mode is not supported.
* Only single-threaded programs are supported for now. The Reactive system relies on `fork()` internally and is not compatible with OCaml's multicore runtime.
* All reactive code must use the provided APIs. Direct access to files, time, network, or non-deterministic sources must be avoided.

This setup ensures that your reactive computations are reproducible and that code and data can be reliably restored across runs.

---

## API Overview

```ocaml
type 'a t
type filename = string
type key = string
type tracker
type 'a marshalled

val init : filename -> int -> unit
val input_files : filename array -> tracker t
val read_file : filename -> tracker -> string
val map : 'a t -> (key -> 'a array -> (key * 'b array) array) -> 'b t
val marshalled_map : 'a t -> (key -> 'a array -> (key * 'b array) array) -> 'b marshalled t
val unmarshal : 'a marshalled -> 'a
val get_array : 'a t -> key -> 'a array
val union : 'a t -> 'a t -> 'a t
val exit : unit -> unit
```

---

## Usage

### 1. Initialize the Cache

```ocaml
Reactive.init "cache.rheap" (1024 * 1024 * 1024);  (* 1 GB heap *)
```

Creates a persistent file to store all cached results. Once the specified size is exceeded, an exception will be raised.

---

### 2. Declare Inputs

```ocaml
let files = [| "input1.txt"; "input2.txt" |] in
let inputs = Reactive.input_files files
```

All I/O must happen via `input_files` and `read_file`. Do not use direct I/O (stdin, real files, network, etc.) as it will break caching guarantees.

---

### 3. Read Files

```ocaml
let contents =
  Reactive.map inputs (fun key trackers ->
    let content = Reactive.read_file key (Array.get trackers 0) in
    [| (key, [| content |]) |]
  )
```

All computations inside `map` are memoized and re-evaluated only if inputs change.

---

### 4. Transform Collections

```ocaml
let processed =
  Reactive.map contents (fun key values ->
    (* your transformation logic *)
    [| (key, values) |]
  )
```

Use `map` for pure computations. The function runs in a forked process, so global mutations do not affect the parent. Forking requires a single-threaded runtime.

You cannot call `map` recursively within a `map`—this will raise an exception to prevent fork bombs.

---

### 5. Marshalled Outputs

If you need to return closures or unsupported types, use `marshalled_map`:

```ocaml
let complex =
  Reactive.marshalled_map contents (fun key values ->
    let result = (* some closure or complex object *) in
    [| (key, [| result |]) |]
  )
```

Note: Marshalled values are less efficient (no sharing, costly deserialization).

---

### 6. Access Results

Use `get_array` inside a `map` or `marshalled_map` to access a specific key:

```ocaml
let result = Reactive.get_array processed "input1.txt"
```

Outside maps, use `exit` first:

```ocaml
Reactive.exit ();
let output = Reactive.get_array processed "input1.txt"
```

---

## Best Practices

* Always interact with data through reactive APIs.
* Avoid using closures, laziness, or mutable objects in `map`.
* Use `marshalled_map` only when absolutely necessary.
* Use `exit` to gradually adopt Reactive in existing codebases.

---

## Limitations

* No support for multi-threading (due to `fork()`).
* Not all OCaml types are supported in `map`.
* No nested maps.
* Requires predefining input files.

---

## License

MIT

---

## Authors

Developed at SkipLabs. This is a proof-of-concept prototype. Feedback and contributions are welcome!
