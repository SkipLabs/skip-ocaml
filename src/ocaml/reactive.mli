type 'a t

type filename = string
type key = string
type tracker
type 'a marshaled

val init : filename -> int -> unit
val input_files : filename array -> tracker t
val read_file : filename -> tracker -> string
val map : 'a t -> (key -> 'a array -> (key * 'b array) array) -> 'b t
val marshaled_map : 'a t -> (key -> 'a array -> (key * 'b array) array) -> 'b marshaled t
val unmarshal : 'a marshaled -> 'a
val marshal : 'a -> 'a marshaled
val get_array : 'a t -> key -> 'a array
val unsafe_get_array : 'a t -> key -> 'a array
val union : 'a t -> 'a t -> 'a t
val exit : unit -> unit
val new_global : 'a -> 'a t
val get_global : 'a t -> 'a
val unsafe_get_global : 'a t -> 'a

