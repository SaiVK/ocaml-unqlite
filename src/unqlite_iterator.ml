
open Unqlite_bindings.Cursor

type entry = string Lazy.t * string Lazy.t

let fold_left f acc db =
  (* TODO disentangle this spaghetti code *)
  let c = init db in
  let rec aux acc =
    if valid_entry c then
      begin
        let acc = f acc (lazy (key c), lazy (data c)) in
        try
          next_entry c;
          aux acc
        with
        | Unqlite_bindings.Unqlite_error (_, code) when code = Unqlite_bindings.UNQLITE_DONE ->
          release db c;
          acc
      end
    else
      begin
        release db c;
        acc
      end
  in
  try
    first_entry c;
    aux acc
  with
  | Unqlite_bindings.Unqlite_error (_, code) when code = Unqlite_bindings.UNQLITE_DONE ->
    acc

let iter f db =
  let f' () kv = f kv in
  fold_left f' () db
