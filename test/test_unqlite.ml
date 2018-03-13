open OUnit
open Fmt

open Unqlite.Unqlite

let open_inmem () = u_open ":mem:"

let test1 _ =
  u_open "/tmp/db.unqlite" |> u_close

let test2 _ =
  let db = u_open "/tmp/db2.unqlite" in
  u_rollback db;
  u_commit db;
  u_close db

let test3 _ =
  let db = open_inmem () in
  u_rollback db;
  u_commit db;
  u_close db

let test4 _ =
  let db = open_inmem () in
  u_store db "foo" "bar";
  assert_equal "bar" (u_fetch db "foo");
  u_close db

let test5 _ =
  assert_raises (Unqlite_error "store failed") @@ fun _ ->
  let db = u_open "/invalid/file" in
  u_store db "foo" "bar"

let test6 _ =
  let key = "foo" in
  let value = "bar" in
  let db = open_inmem () in
  assert_raises Not_found (fun _ -> u_fetch db key);
  u_store db key value;
  assert_equal value(u_fetch db key);
  u_close db


let suite =
  "unqlite suite" >::: [ "test1" >:: test1
                       ; "test2" >:: test2
                       ; "test3" >:: test3
                       ; "test4" >:: test4
                       ; "test5" >:: test5
                       ; "test6" >:: test6
                       ]

let _ =
  run_test_tt_main suite
