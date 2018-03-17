type t

exception Unqlite_error of string

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
