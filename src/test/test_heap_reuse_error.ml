let is_darwin () =
  try
    let ic = Unix.open_process_in "uname -s" in
    match input_line ic with
    | line ->
        ignore (Unix.close_process_in ic);
        String.trim line = "Darwin"
  with _ -> false

let fresh_heap_path () =
  let path = Filename.temp_file "skip_reuse" ".rheap" in
  Sys.remove path;
  path

let () =
  if Array.length Sys.argv >= 3 && Sys.argv.(1) = "--child" then (
    let heap = Sys.argv.(2) in
    Reactive.init heap (1024 * 1024);
    Reactive.exit ();
    Printf.eprintf
      "Unexpectedly succeeded in reusing persistent heap %s\n"
      heap;
    exit 0
  );

  if not (is_darwin ()) then (
    print_endline "skipping heap reuse rejection test (non-macOS platform)";
    exit 0
  );

  let heap = fresh_heap_path () in
  let cleanup () = if Sys.file_exists heap then Sys.remove heap in
  Fun.protect
    ~finally:cleanup
    (fun () ->
       let size = 1024 * 1024 in
       Reactive.init heap size;
       Reactive.exit ();

       let cmd = Sys.argv.(0) in
       let child =
         Unix.create_process
           cmd
           [| cmd; "--child"; heap |]
           Unix.stdin
           Unix.stdout
           Unix.stderr
       in
       match snd (Unix.waitpid [] child) with
       | Unix.WEXITED 0 ->
           failwith "Heap reuse unexpectedly succeeded on macOS"
       | Unix.WEXITED code ->
           Printf.printf
             "heap reuse correctly rejected with exit code %d\n"
             code
       | Unix.WSIGNALED signal ->
           Printf.printf
             "heap reuse correctly rejected via signal %d\n"
             signal
       | Unix.WSTOPPED signal ->
           Printf.printf
             "heap reuse stopped by signal %d (treated as rejection)\n"
             signal)

