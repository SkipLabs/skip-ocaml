let write_temp () =
  let path = Filename.temp_file "skip_pure_map" ".txt" in
  let oc = open_out path in
  output_string oc "42";
  close_out oc;
  path

let () =
  let fname = write_temp () in
  Fun.protect
    ~finally:(fun () -> if Sys.file_exists fname then Sys.remove fname)
    (fun () ->
      Reactive.init "test_cache_map.rheap" (1024 * 1024);
      let t = Reactive.input_files [| fname |] in

      let _ = Reactive.map t (fun key _ ->
        let x = 42 in
        [| (key, [| x, x + 1 |]) |]  (* tuple, safe *)
      ) in

      print_endline "pure map passed"
    )
