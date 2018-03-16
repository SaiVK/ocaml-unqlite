open OUnit
open Fmt

open Unqlite.Bindings

(* TODO: remove hard-coded file locations *)

let assert_error_msg_contains s =
  let pattern = Str.quote s |> Str.regexp in
  fun f ->
    try f () with
    | Unqlite_error msg ->
      try
        ignore (Str.search_forward pattern msg 0)
      with
      | Not_found -> failwith "error message [%s] did not contain [%s]" msg s

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
  let db = open_inmem () in
  store db "foo" "bar";
  assert_equal "bar" (fetch db "foo");
  close db

let test5a _ =
  assert_error_msg_contains "IO error" @@ fun _ ->
  let db = open_create "/invalid/file" in
  store db "foo" "bar"

let test5b _ =
  let name = "/tmp/db.5b.unqlite" in
  let (key, value) = "5b", "b5" in

  let db = open_readwrite name in
  assert_error_msg_contains "IO error" (fun _ -> store db key value);
  close db;

  let db = open_create name in
  store db key value;
  close db;

  let db = open_mmap name in
  assert_error_msg_contains "Read-only" (fun _ -> store db key value);
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
  assert_equal value(fetch db key);
  close db


let suite =
  "unqlite suite" >::: [ "test1" >:: test1
                       ; "test2" >:: test2
                       ; "test3" >:: test3
                       ; "test4" >:: test4
                       ; "test5a" >:: test5a
                       ; "test5b" >:: test5b
                       ; "test6" >:: test6
                       ]

let _ =
  run_test_tt_main suite
