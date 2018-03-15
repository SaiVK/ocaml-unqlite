type t

exception Unqlite_error of string
let _ =
  Callback.register_exception "de.burgerdev.unqlite.error" (Unqlite_error "")

(* TODO: support open flags *)
external u_open: string -> t = "o_unqlite_open"
external u_close: t -> unit = "o_unqlite_close"

external u_commit: t -> unit = "o_unqlite_commit"
external u_rollback: t -> unit = "o_unqlite_rollback"

external u_store: t -> string -> string -> unit = "o_unqlite_kv_store"
external u_fetch: t -> string -> string = "o_unqlite_kv_fetch"
