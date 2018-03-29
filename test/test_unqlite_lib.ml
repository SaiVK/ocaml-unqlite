open OUnit
open Fmt

open Unqlite

let test_fold_left _ =
  let db = open_db ":mem:" in
  store db "foo" "bar";
  store db "baz" "qux";
  let list_acc acc (lazy_key, lazy_val) =
    if Lazy.force lazy_key = "baz" then
      Lazy.force lazy_val :: acc
    else
      acc
  in
  let acc = Iterator.fold_left list_acc [] db in
  assert_equal ["qux"] acc


let suite =
  "unqlite suite" >::: [ "fold_left" >:: test_fold_left
                       ]

let _ =
  run_test_tt_main suite
