type t

(* TODO: support all open flags *)
external open_create: string -> t = "o_unqlite_open_create"
external open_readwrite: string -> t = "o_unqlite_open_readwrite"
external open_mmap: string -> t = "o_unqlite_open_mmap"
external close: t -> unit = "o_unqlite_close"

external commit: t -> unit = "o_unqlite_commit"
external rollback: t -> unit = "o_unqlite_rollback"

external store: t -> string -> string -> unit = "o_unqlite_kv_store"
external append: t -> string -> string -> unit = "o_unqlite_kv_append"
external fetch: t -> string -> string = "o_unqlite_kv_fetch"
external delete: t -> string -> unit = "o_unqlite_kv_delete"

module Cursor = struct
  type c

  external init: t -> c = "o_unqlite_cursor_init"
  external release: t -> c -> unit = "o_unqlite_cursor_release"

  external first_entry: c -> unit = "o_unqlite_cursor_first_entry"
  external next_entry: c -> unit = "o_unqlite_cursor_next_entry"
  external valid_entry: c -> bool = "o_unqlite_cursor_valid_entry"

  external key: c -> string = "o_unqlite_cursor_key"
  external data: c -> string = "o_unqlite_cursor_data"

end

type error_code =
  | UNQLITE_NOMEM            (* Out of memory *)
  | UNQLITE_ABORT            (* Another thread have released this instance *)
  | UNQLITE_IOERR            (* IO error *)
  | UNQLITE_CORRUPT          (* Corrupt pointer *)
  | UNQLITE_LOCKED           (* Forbidden Operation *)
  | UNQLITE_BUSY            	(* The database file is locked *)
  | UNQLITE_DONE            	(* Operation done *)
  | UNQLITE_PERM             (* Permission error *)
  | UNQLITE_NOTIMPLEMENTED   (* Method not implemented by the underlying Key/Value storage engine *)
  | UNQLITE_NOTFOUND         (* No such record *)
  | UNQLITE_NOOP             (* No such method *)
  | UNQLITE_INVALID          (* Invalid parameter *)
  | UNQLITE_EOF              (* End Of Input *)
  | UNQLITE_UNKNOWN          (* Unknown configuration option *)
  | UNQLITE_LIMIT            (* Database limit reached *)
  | UNQLITE_EXISTS           (* Records exists *)
  | UNQLITE_EMPTY            (* Empty record *)
  | UNQLITE_COMPILE_ERR      (* Compilation error *)
  | UNQLITE_VM_ERR           (* Virtual machine error *)
  | UNQLITE_FULL             (* Full database (unlikely) *)
  | UNQLITE_CANTOPEN         (* Unable to open the database file *)
  | UNQLITE_READ_ONLY        (* Read only Key/Value storage engine *)
  | UNQLITE_LOCKERR          (* Locking protocol error *)
  | UNQLITE_BINDINGS         (* Error in the stub code *)

exception Unqlite_error of string * error_code
let _ =
  Callback.register_exception
    "de.burgerdev.unqlite.error"
    (Unqlite_error ("", UNQLITE_BINDINGS))
