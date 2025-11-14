let write_temp prefix content =
  let path = Filename.temp_file prefix ".txt" in
  let oc = open_out path in
  output_string oc content;
  close_out oc;
  path

let () =
  let f1 = write_temp "skip_incr_double_a" "first file" in
  let f2 = write_temp "skip_incr_double_b" "second file" in
  Fun.protect
    ~finally:(fun () ->
      List.iter (fun file -> if Sys.file_exists file then Sys.remove file) [f1; f2]
    )
    (fun () ->
      Reactive.init "test_incr_double_file.rheap" (1024 * 1024);

      let inputs = Reactive.input_files [| f1; f2 |] in

      let contents =
        Reactive.map inputs (fun key trackers ->
          let content = Reactive.read_file key (Array.get trackers 0) in
          [| (key, [| content |]) |]
        )
      in

      let lengths =
        Reactive.map contents (fun key arr ->
          [| (key, [| String.length arr.(0) |]) |]
        )
      in

      Reactive.exit ();
      let a = Reactive.get_array contents f1 in
      let b = Reactive.get_array contents f2 in
      let la = Reactive.get_array lengths f1 in
      let lb = Reactive.get_array lengths f2 in

      Printf.printf "%s [%d] / %s [%d]\n" a.(0) la.(0) b.(0) lb.(0)
    )
