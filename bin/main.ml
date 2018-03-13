open Unix

let bin_name = "unqlite"
let bin_version = "0.1"

let store db key value _ =
  Logs.debug (fun m -> m "Storing key [%s] in database [%s]." key db);
  let open Unqlite.Unqlite in
  let db = u_open db in
  try
    u_store db key value;
    u_close db
  with
  | e -> u_close db; raise e

let fetch db_name key _ =
  Logs.debug (fun m -> m "Fetching key [%s] from database [%s]." key db_name);
  let open Unqlite.Unqlite in
  let db = u_open db_name in
  begin
    try
      Printf.printf "%s\n" @@ u_fetch db key
    with
    | Not_found ->
      Logs.err (fun m -> m "Key [%s] does not exist in database [%s]." key db_name)
  end;
  u_close db



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

let fetch_cmd =
  let doc = "Fetch a value from the database." in
  Term.(const fetch $ db_term $ key_term $ log_term),
  Term.info "fetch" ~doc ~sdocs:Manpage.s_common_options

let store_cmd =
  let doc = "Store a key/value pair in the database." in
  Term.(const store $ db_term $ key_term $ value_term $ log_term),
  Term.info "store" ~doc ~sdocs:Manpage.s_common_options

let default_cmd =
  let doc = "Unqlite key/value store" in
  Term.(ret (const (fun _ -> `Help (`Pager, None)) $ log_term)),
  Term.info bin_name ~version:bin_version ~doc

let cmds = [fetch_cmd; store_cmd]

let () = Term.(exit @@ eval_choice default_cmd cmds)
