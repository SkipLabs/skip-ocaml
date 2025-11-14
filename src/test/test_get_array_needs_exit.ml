let write_temp content =
  let path = Filename.temp_file "skip_get_array_needs_exit" ".txt" in
  let oc = open_out path in
  output_string oc content;
  close_out oc;
  path

let () =
  let fname = write_temp "pending" in
  Fun.protect
    ~finally:(fun () -> if Sys.file_exists fname then Sys.remove fname)
    (fun () ->
      Reactive.init "test_cache_get.rheap" (1024 * 1024);
      let t = Reactive.input_files [| fname |] in
      let m = Reactive.map t (fun key _ -> [| (key, [| key |]) |]) in

      let failed =
        try
          let _ = Reactive.get_array m fname in
          false
        with _ -> true
      in
      assert failed;
      print_endline "get_array before exit fails as expected"
    )
