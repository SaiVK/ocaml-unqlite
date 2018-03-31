
type entry = string Lazy.t * string Lazy.t

val fold_left: ('a -> entry -> 'a) -> 'a -> Unqlite_bindings.t -> 'a

val iter: (entry -> unit) -> Unqlite_bindings.t -> unit
