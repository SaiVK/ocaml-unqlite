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
