
module Bindings = Unqlite_bindings

type t = Unqlite_bindings.t

let fetch_exn = Bindings.fetch
let fetch t key =
  try
    Some (fetch_exn t key)
  with
  | Not_found -> None

let store = Bindings.store
let append = Bindings.append

let delete_exn = Bindings.delete
let delete t key =
  try
    ignore (delete_exn t key)
  with
  | Not_found -> ()

type open_mode =
  | Create
  | Read_write
  | MMap

let open_db ?(mode=Create) name = match mode with
  | Create -> Bindings.open_create name
  | Read_write -> Bindings.open_readwrite name
  | MMap -> Bindings.open_mmap name
let close = Bindings.close

let commit = Bindings.commit
let rollback = Bindings.rollback
