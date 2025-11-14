let write_temp prefix content =
  let path = Filename.temp_file prefix ".txt" in
  let oc = open_out path in
  output_string oc content;
  close_out oc;
  path

let cleanup files =
  List.iter (fun file -> if Sys.file_exists file then Sys.remove file) files

let () =
  let file_a = write_temp "skip_basic2_a" "alpha\nbeta\n" in
  let file_b = write_temp "skip_basic2_b" "gamma" in
  let files = [| file_a; file_b |] in
  Fun.protect
    ~finally:(fun () -> cleanup (Array.to_list files))
    (fun () ->
      Reactive.init "cache.rheap" (1024 * 1024 * 1024);  (* 1 GB heap *)
      let inputs = Reactive.input_files files in
      let contents =
        Reactive.map inputs (fun key trackers ->
          print_endline key;
          let content = Reactive.read_file key (Array.get trackers 0) in
          [| (key ^ "-content", [| content |]) |]
        )
      in
      let lengths =
        Reactive.map contents (fun key contents ->
          print_endline key;
          let length = String.length (Array.get contents 0) in
          [| (key ^ "-length", [| length |]) |]
        )
      in
      Reactive.exit ();
      files |> Array.iter (fun file ->
        let length = (Reactive.get_array lengths (file ^ "-content-length")).(0) in
        print_endline (Printf.sprintf "%s: %d" file length)
      )
    )
