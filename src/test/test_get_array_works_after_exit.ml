let () =
  let content = "ok" in
  let fname = Filename.temp_file "skip_after_exit" ".txt" in
  let oc = open_out fname in
  output_string oc content;
  close_out oc;
  Fun.protect
    ~finally:(fun () -> if Sys.file_exists fname then Sys.remove fname)
    (fun () ->
      Reactive.init "test_cache_exit.rheap" (1024 * 1024);
      let t = Reactive.input_files [| fname |] in
      let m =
        Reactive.map t (fun key trackers ->
          let s = Reactive.read_file key (Array.get trackers 0) in
          [| (key, [| s |]) |]
        )
      in
      Reactive.exit ();
      let out = Reactive.get_array m fname in
      assert (out.(0) = content);
      print_endline "get_array after exit passed"
    )
