type t

exception Unqlite_error of string

val u_open: string -> t
val u_close: t -> unit

val u_commit: t -> unit
val u_rollback: t -> unit

val u_store: t -> string -> string -> unit
val u_fetch: t -> string -> string
