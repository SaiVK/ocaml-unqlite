
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

module Iterator = struct
  open Bindings.Cursor

  type entry = string Lazy.t * string Lazy.t

  let fold_left f acc db =
    let c = init db in
    first_entry c;
    let rec aux acc =
      if valid_entry c then
        begin
          let acc = f acc (lazy (key c), lazy (data c)) in
          next_entry c;
          aux acc
        end
      else
        begin
          release db c;
          acc
        end
    in aux acc

  let iter f db =
    let f' () kv = f kv in
    fold_left f' () db

end
