open OUnit
open Fmt

open Unqlite.Bindings

(* TODO: remove hard-coded file locations *)

let assert_raises_with_msg s =
  let pattern = Str.quote s |> Str.regexp in
  fun f ->
    try
      f ();
      failwith "no error raised"
    with
    | Unqlite_error (msg, _) ->
      try
        ignore (Str.search_forward pattern msg 0)
      with
      | Not_found -> failwith "error message [%s] did not contain [%s]" msg s

let assert_raises_with_code c =
  fun f ->
    try
      f ();
      failwith "no error raised"
    with
    | Unqlite_error (_, code) as exn ->
      if code != c then
        let expected = Printexc.to_string (Unqlite_error ("", c)) in
        let actual = Printexc.to_string exn in
        failwith "expected error [%s], got [%s]" expected actual

let open_inmem () = open_create ":mem:"

let test1 _ =
  open_create "/tmp/db.unqlite" |> close

let test2 _ =
  let db = open_create "/tmp/db2.unqlite" in
  rollback db;
  commit db;
  close db

let test3 _ =
  let db = open_inmem () in
  rollback db;
  commit db;
  close db

let test4 _ =
  let (key, value) = "foo", "bar" in
  let db = open_inmem () in
  store db key value;
  assert_equal value (fetch db key);
  delete db key;
  assert_raises Not_found (fun _ -> fetch db "foo");
  close db

let test5a _ =
  assert_raises_with_msg "IO error" @@ fun _ ->
  let db = open_create "/invalid/file" in
  store db "foo" "bar"

let test5b _ =
  let name = "/tmp/db.5b.unqlite" in
  let (key, value) = "5b", "b5" in

  let db = open_readwrite name in
  assert_raises_with_msg "IO error" (fun _ -> store db key value);
  assert_raises_with_code UNQLITE_IOERR (fun _ -> store db key value);
  close db;

  let db = open_create name in
  store db key value;
  close db;

  let db = open_mmap name in
  assert_raises_with_msg "Read-only" (fun _ -> store db key value);
  assert_raises_with_code UNQLITE_READ_ONLY (fun _ -> store db key value);
  close db;

  let db = open_readwrite name in
  let pp_diff = Fmt.(pair ~sep:(const string " != ") (string) (string)) in
  assert_equal ~pp_diff value (fetch db key);
  close db

let test6 _ =
  let key = "foo" in
  let value = "bar" in
  let db = open_inmem () in
  assert_raises Not_found (fun _ -> fetch db key);
  store db key value;
  assert_equal value (fetch db key);
  append db key value;
  assert_equal (String.concat "" [value; value]) (fetch db key);
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
  "unqlite suite" >::: [ "test1" >:: test1
                       ; "test2" >:: test2
                       ; "test3" >:: test3
                       ; "test4" >:: test4
                       ; "test5a" >:: test5a
                       ; "test5b" >:: test5b
                       ; "test6" >:: test6
                       ; "test_cursor" >:: test_cursor
                       ]

let _ =
  run_test_tt_main suite
