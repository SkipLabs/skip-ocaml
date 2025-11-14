let write_temp () =
  let path = Filename.temp_file "skip_incr_chain" ".txt" in
  let oc = open_out path in
  output_string oc "incremental chain";
  close_out oc;
  path

let () =
  let fname = write_temp () in
  Fun.protect
    ~finally:(fun () -> if Sys.file_exists fname then Sys.remove fname)
    (fun () ->
      Reactive.init "test_incr_chain.rheap" (1024 * 1024);

      let inputs = Reactive.input_files [| fname |] in

      let contents =
        Reactive.map inputs (fun key trackers ->
          let s = Reactive.read_file key (Array.get trackers 0) in
          [| (key, [| s |]) |]
        )
      in

      let upper =
        Reactive.map contents (fun key arr ->
          let s = String.uppercase_ascii arr.(0) in
          [| (key, [| s |]) |]
        )
      in

      let reversed =
        Reactive.map upper (fun key arr ->
            let s =
              let orig = arr.(0) in
              let len = String.length orig in
              String.init len (fun i -> orig.[len - i - 1])
            in
          [| (key, [| s |]) |]
        )
      in

      Reactive.exit ();
      let out = Reactive.get_array reversed fname in
      Printf.printf "Final: %s\n" out.(0)
    )
