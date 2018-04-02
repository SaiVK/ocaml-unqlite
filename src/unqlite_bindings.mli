type t

(* TODO: the [open_] API is only a temporary solution, should be replaced by
         the proper stub (taking flags). *)
val open_create: string -> t
val open_readwrite: string -> t
val open_mmap: string -> t

val close: t -> unit

val commit: t -> unit
val rollback: t -> unit

val store: t -> string -> string -> unit
val append: t -> string -> string -> unit
val fetch: t -> string -> string
val delete: t -> string -> unit

module Cursor: sig
  type c

  val init: t -> c
  val release: t -> c -> unit

  val first_entry: c -> unit
  val next_entry: c -> unit
  val valid_entry: c -> bool

  val key: c -> string
  val data: c -> string
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

val pp_error_code: Format.formatter -> error_code -> unit
