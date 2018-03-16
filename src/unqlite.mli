
module Bindings: module type of Unqlite_bindings

type t = Bindings.t

type open_mode =
  | Create
  | Read_write
  | MMap

val open_db: ?mode:open_mode -> string -> t
val close: t -> unit

val fetch: t -> string -> string option
val fetch_exn: t -> string -> string

val delete: t -> string -> unit
val delete_exn: t -> string -> unit

val store: t -> string -> string -> unit

val commit: t -> unit
val rollback: t -> unit
