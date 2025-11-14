let write_temp () =
  let path = Filename.temp_file "skip_map_closure" ".txt" in
  let oc = open_out path in
  output_string oc "number 1";
  close_out oc;
  path

let () =
  let fname = write_temp () in
  Fun.protect
    ~finally:(fun () -> if Sys.file_exists fname then Sys.remove fname)
    (fun () ->
      Reactive.init "test_cache_closure.rheap" (1024 * 1024);
      let t = Reactive.input_files [| fname |] in

      let failed =
        try
          let _ = Reactive.map t (fun key _ ->
            let closure = (fun x -> x + 1) in
            [| (key, [| closure |]) |]
          ) in
          false
        with _ -> true
      in
      assert failed;
      print_endline "closure rejected as expected"
    )
