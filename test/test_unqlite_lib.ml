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
  assert_equal ["qux"] acc;
  close db

let test_empty_iter_disk _ =
  let db = open_db @@ Filename.temp_file "ocaml_unqlite_" ".db" in
  Iterator.iter (fun _ -> ()) db

let test_empty_iter_mem _ =
  let db = open_db ":mem:" in
  Iterator.iter (fun _ -> ()) db

let test_iter db =
  let map = Hashtbl.create 2 in
  store db "foo" "bar";
  store db "baz" "qux";
  let map_push (lazy_key, lazy_val) =
    Hashtbl.add map (Lazy.force lazy_key) (Lazy.force lazy_val)
  in
  Iterator.iter map_push db;
  close db;
  assert_equal ("bar") (Hashtbl.find map "foo");
  assert_equal ("qux") (Hashtbl.find map "baz");
  assert_equal 2 (Hashtbl.length map)

let test_iter_mem _ =
  let db = open_db ":mem:" in
  test_iter db

let test_iter_disk _ =
  let db = open_db @@ Filename.temp_file "ocaml_unqlite_" ".db" in
  test_iter db


let suite =
  "unqlite suite" >::: [ "fold_left" >:: test_fold_left
                       ; "empty_iter_mem" >:: test_empty_iter_mem
                       ; "iter_mem" >:: test_iter_mem
                       ; "empty_iter_disk" >:: test_empty_iter_disk
                       ; "iter_disk" >:: test_iter_disk
                       ]

let _ =
  run_test_tt_main suite
