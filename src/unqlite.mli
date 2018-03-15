
module Bindings: module type of Unqlite_bindings

type t = Bindings.t

val open_rw: string -> t
val close: t -> unit

val fetch: t -> string -> string option
val fetch_exn: t -> string -> string

val store: t -> string -> string -> unit

val commit: t -> unit
val rollback: t -> unit
