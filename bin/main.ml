open Unix

let bin_name = "unqlite"
let bin_version = "0.1"

open Unqlite

let store db_name key value _ =
  Logs.debug (fun m -> m "Storing key [%s] in database [%s]." key db_name);
  let db = open_db db_name in
  try
    store db key value;
    close db
  with
  | e -> close db; raise e

let fetch db_name key _ =
  Logs.debug (fun m -> m "Fetching key [%s] from database [%s]." key db_name);
  let db = open_db db_name in

  try
    fetch_exn db key |> print_endline;
    close db
  with
  | e -> close db; raise e

let delete db_name key _ =
  Logs.debug (fun m -> m "Deleting key [%s] from database [%s]." key db_name);
  let db = open_db db_name in
  try
    delete_exn db key;
    close db
  with
  | Not_found -> close db
  | e -> close db; raise e

let append db_name key value _ =
  Logs.debug (fun m -> m "Appending key [%s] to database [%s]." key db_name);
  let db = open_db db_name in
  try
    append db key value;
    close db
  with
  | e -> close db; raise e


(* Logging stuff, copy pasta from docs *)

let setup_log style_renderer level =
  Fmt_tty.setup_std_outputs ?style_renderer ();
  Logs.set_level level;
  Logs.set_reporter (Logs_fmt.reporter ());
  ()

(* Command line interface *)

open Cmdliner

let log_term =
  Term.(const setup_log $ Fmt_cli.style_renderer () $ Logs_cli.level ())

let db_term =
  let doc = "Backing unqlite database file." in
  let env = Arg.env_var "UNQLITE_DATABASE" ~doc in
  Arg.(value & opt string ":mem:" & info ["d"; "database"] ~env ~docv:"DB-FILE" ~doc)

let key_term =
  let doc = "Key string (length must fit into int32)." in
  Arg.(required & pos 0 (some string) None & info [] ~docv:"KEY" ~doc)

let value_term =
  let doc = "Value string (length must fit into int64)." in
  Arg.(required & pos 1 (some string) None & info [] ~docv:"VALUE" ~doc)

(* Cmdliner stuff *)

let tag doc =
  let word = match String.index_opt doc ' ' with
    | None -> doc
    | Some n -> String.sub doc 0 n
  in String.lowercase_ascii word

let unary_cmd f t doc =
  Term.(const f $ db_term $ t $ log_term),
  Term.info (tag doc) ~doc ~sdocs:Manpage.s_common_options

let binary_cmd f t u doc =
  Term.(const f $ db_term $ t $ u $ log_term),
  Term.info (tag doc) ~doc ~sdocs:Manpage.s_common_options

let default_cmd =
  let doc = "Unqlite key/value store" in
  Term.(ret (const (fun _ -> `Help (`Pager, None)) $ log_term)),
  Term.info bin_name ~version:bin_version ~doc

let cmds = [
  unary_cmd fetch key_term "Fetch a value from the database.";
  binary_cmd store key_term value_term "Store a key/value pair in the database.";
  binary_cmd append key_term value_term "Append a key/value pair to the database.";
  unary_cmd delete key_term "Delete a key/value pair from the database (if it exists)."
]

let () = Term.(exit @@ eval_choice default_cmd cmds)
