let write_temp () =
  let path = Filename.temp_file "skip_nested_map" ".txt" in
  let oc = open_out path in
  output_string oc "payload";
  close_out oc;
  path

let () =
  let fname = write_temp () in
  Fun.protect
    ~finally:(fun () -> if Sys.file_exists fname then Sys.remove fname)
    (fun () ->
      Reactive.init "test_cache_nested.rheap" (1024 * 1024);
      let t = Reactive.input_files [| fname |] in

      let failed =
        try
          let _ = Reactive.map t (fun key _ ->
            let _ = Reactive.map t (fun _ _ -> [||]) in
            [| (key, [| 1 |]) |]
          ) in
          false
        with _ -> true
      in
      assert failed;
      print_endline "nested map rejected"
    )
