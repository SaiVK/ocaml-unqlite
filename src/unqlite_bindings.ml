type t

exception Unqlite_error of string
let _ =
  Callback.register_exception "de.burgerdev.unqlite.error" (Unqlite_error "")

(* TODO: support all open flags *)
external open_create: string -> t = "o_unqlite_open_create"
external open_readwrite: string -> t = "o_unqlite_open_readwrite"
external open_mmap: string -> t = "o_unqlite_open_mmap"
external close: t -> unit = "o_unqlite_close"

external commit: t -> unit = "o_unqlite_commit"
external rollback: t -> unit = "o_unqlite_rollback"

external store: t -> string -> string -> unit = "o_unqlite_kv_store"
external fetch: t -> string -> string = "o_unqlite_kv_fetch"
external delete: t -> string -> unit = "o_unqlite_kv_delete"
