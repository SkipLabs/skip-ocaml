let write_temp () =
  let path = Filename.temp_file "skip_incr_aggregate" ".txt" in
  let oc = open_out path in
  output_string oc "hello world reactive aggregate";
  close_out oc;
  path

let () =
  let fname = write_temp () in
  Fun.protect
    ~finally:(fun () -> if Sys.file_exists fname then Sys.remove fname)
    (fun () ->
      Reactive.init "test_incr_aggregate.rheap" (1024 * 1024);

      let inputs = Reactive.input_files [| fname |] in

      let words =
        Reactive.map inputs (fun key trackers ->
          let content = Reactive.read_file key (Array.get trackers 0) in
          let ws = String.split_on_char ' ' content in
          Array.of_list (List.map (fun w -> (w, [| w |])) ws)
        )
      in

      let lengths =
        Reactive.map words (fun key arr ->
          [| (key, [| String.length key |]) |]
        )
      in

      Reactive.exit ();
      let count = Array.length (Reactive.get_array lengths "hello") in
      Printf.printf "Length for key 'hello': %d\n" count
    )
