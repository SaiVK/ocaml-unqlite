type t

exception Unqlite_error of string

val open_create: string -> t
val open_readwrite: string -> t
val open_mmap: string -> t
val close: t -> unit

val commit: t -> unit
val rollback: t -> unit

val store: t -> string -> string -> unit
val fetch: t -> string -> string
