let write_temp () =
  let path = Filename.temp_file "skip_marshal_closure" ".txt" in
  let oc = open_out path in
  output_string oc "marshal me";
  close_out oc;
  path

let () =
  let fname = write_temp () in
  Fun.protect
    ~finally:(fun () -> if Sys.file_exists fname then Sys.remove fname)
    (fun () ->
      Reactive.init "test_cache_marsh.rheap" (1024 * 1024);
      let t = Reactive.input_files [| fname |] in

      let m = Reactive.marshalled_map t (fun key _ ->
        let closure = (fun x -> x ^ "!") in
        [| (key, [| closure |]) |]
      ) in
      Reactive.exit ();
      let arr = Reactive.get_array m fname in
      let f = Reactive.unmarshal arr.(0) in
      assert (f "wow" = "wow!");
      print_endline "marshalled closure passed"
    )
