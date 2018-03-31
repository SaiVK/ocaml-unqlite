
open Unqlite_bindings.Cursor

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
