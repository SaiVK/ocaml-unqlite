open OUnit
open Fmt

open Unqlite.Bindings

let assert_raises_unqlite ?(code=None) regexp =
  let none = Fmt.(const string "_") in
  let expected =
    Fmt.(strf "Unqlite_error (%S, %a)" regexp (option ~none pp_error_code) code)
  in
  let pattern = Str.regexp regexp in
  fun f ->
    try
      f ();
      failwith "expected error matching [%s], but no error was raised" expected
    with
    | Unqlite_error (msg, rc) as exn ->
      let actual = Printexc.to_string exn in
      let fail () =
        failwith "expected error matching [%s], got [%s]" expected actual
      in
      begin
        match code with
        | Some c when c != rc -> fail ()
        | _ -> ()
      end;
      try
        ignore (Str.search_forward pattern msg 0)
      with
      | Not_found ->
        fail()

let temp_db opener =
  Filename.temp_file "ocaml_unqlite_" ".db" |> opener

let open_inmem () = open_create ":mem:"

let test_open _ =
  temp_db open_create |> close

let test_basics_disk _ =
  let db = temp_db open_create in
  rollback db;
  commit db;
  close db

let test_basics_mem _ =
  let db = open_inmem () in
  rollback db;
  commit db;
  close db

let test_crud db =
  let (key, value, value2) = "foo", "bar", "baz" in
  store db key value;
  assert_equal value (fetch db key);
  append db key value;
  assert_equal (String.concat "" [value; value]) (fetch db key);
  store db key value2;
  assert_equal value2 (fetch db key);
  delete db key;
  assert_raises Not_found (fun _ -> fetch db "foo");
  close db

let test_crud_disk _ =
  let db = open_inmem () in
  test_crud db

let test_crud_mem _ =
  let db = open_inmem () in
  test_crud db

let test_invalid_file _ =
  assert_raises_unqlite ~code:(Some UNQLITE_IOERR) "IO error" @@ fun _ ->
  let db = open_create "/invalid/file/burgerdev.de/ocaml-unqlite" in
  store db "foo" "bar"

let test_open_readwrite _ =
  let _ = Filename.temp_file  in
  let name = temp_db @@ fun x -> x in
  let (key, value) = "5b", "b5" in

  (* the temp file is already created, need to remove it to check [open_readwrite] *)
  Unix.unlink name;

  let db = open_readwrite name in
  assert_raises_unqlite ~code:(Some UNQLITE_IOERR) "IO error" (fun _ -> store db key value);
  close db;

  let db = open_create name in
  store db key value;
  close db;

  let db = open_mmap name in
  assert_raises_unqlite ~code:(Some UNQLITE_READ_ONLY) "Read-only" (fun _ -> store db key value);
  close db;

  let db = open_readwrite name in
  let pp_diff = Fmt.(pair ~sep:(const string " != ") (string) (string)) in
  assert_equal ~pp_diff value (fetch db key);
  close db

let test_cursor _ =
  let key = "foo" in
  let value = "bar" in
  let db = open_inmem () in
  let c = Cursor.init db in
  Cursor.first_entry c;
  assert (not @@ Cursor.valid_entry c);
  store db key value;
  Cursor.first_entry c;
  assert (Cursor.valid_entry c);
  assert_equal key Cursor.(key c);
  assert_equal value Cursor.(data c);
  Cursor.next_entry c;
  assert (not @@ Cursor.valid_entry c);
  Cursor.release db c;
  close db


let suite =
  "unqlite suite" >::: [ "test_open" >:: test_open
                       ; "test_basics_disk" >:: test_basics_disk
                       ; "test_basics_mem" >:: test_basics_mem
                       ; "test_crud_disk" >:: test_crud_disk
                       ; "test_crud_mem" >:: test_crud_mem
                       ; "test_invalid_file" >:: test_invalid_file
                       ; "test_open_readwrite" >:: test_open_readwrite
                       ; "test_cursor" >:: test_cursor
                       ]

let _ =
  run_test_tt_main suite
