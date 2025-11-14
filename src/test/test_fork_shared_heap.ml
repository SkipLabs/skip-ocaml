let write_temp_file prefix content =
  let path = Filename.temp_file prefix ".txt" in
  let oc = open_out path in
  output_string oc content;
  close_out oc;
  path

let fresh_heap_path () =
  let path = Filename.temp_file "skip_fork_heap" ".rheap" in
  Sys.remove path;
  path

let () =
  let heap = fresh_heap_path () in
  let input = write_temp_file "skip_fork_input" "forking works" in
  let cleanup () =
    List.iter
      (fun file -> if Sys.file_exists file then Sys.remove file)
      [ heap; input ]
  in
  Fun.protect
    ~finally:cleanup
    (fun () ->
       Reactive.init heap (1024 * 1024);

       let inputs = Reactive.input_files [| input |] in
       let contents =
         Reactive.map inputs (fun key trackers ->
             let data = Reactive.read_file key (Array.get trackers 0) in
             [| (key, [| data |]) |])
       in
       let lengths =
         Reactive.map contents (fun key values ->
             let len = String.length values.(0) in
             [| (key, [| len |]) |])
       in

       Reactive.exit ();

       match Unix.fork () with
       | 0 ->
           let arr = Reactive.get_array lengths input in
           Printf.printf
             "child read cached length: %d\n"
             arr.(0);
           exit 0
       | pid ->
           match snd (Unix.waitpid [] pid) with
           | Unix.WEXITED 0 ->
               print_endline "forked child reused persistent heap successfully"
           | Unix.WEXITED code ->
               Printf.eprintf "Child exited with code %d\n" code;
               exit 1
           | Unix.WSIGNALED signal ->
               Printf.eprintf "Child killed by signal %d\n" signal;
               exit 1
           | Unix.WSTOPPED signal ->
               Printf.eprintf "Child stopped by signal %d\n" signal;
               exit 1)

