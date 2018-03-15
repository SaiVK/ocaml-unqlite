
module Bindings = Unqlite_bindings
open Bindings

type t = Unqlite_bindings.t

let fetch_exn = u_fetch
let fetch t key =
  try
    Some (fetch_exn t key)
  with
  | Not_found -> None

let store = u_store

let open_rw = u_open
let close = u_close

let commit = u_commit
let rollback = u_rollback
