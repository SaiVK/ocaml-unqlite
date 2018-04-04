#include <limits.h>

#define CAML_NAME_SPACE

#include <caml/memory.h>
#include <caml/mlvalues.h>
#include <caml/callback.h>
#include <caml/fail.h>
#include <caml/alloc.h>

#include "unqlite.h"

// disabling JX9 fails on mac because [SyStrncmp] is used even though it's disabled
// -> if we're on Mac and JX9 is disabled, we provide our own implementation
#ifdef __APPLE__
#ifdef JX9_DISABLE_BUILTIN_FUNC
#include <string.h>
extern int SyStrncmp(const char* a, const char* b, unsigned long n) {
  return strncmp(a, b, n);
}
#endif
#endif

#define OCAML_UNQLITE_ERROR_UNKNOWN 23
#define UNQLITE_UNDEFINED_ERROR 0x00ffffff
static const char *OCAML_UNQLITE_ERROR = "de.burgerdev.unqlite.error";

static int ocaml_return_code(int);

inline static void o_raise(const char *msg, int code) {
  CAMLparam0();
  CAMLlocal2(ocaml_msg, ocaml_code);

  ocaml_msg = caml_copy_string(msg);
  ocaml_code = Val_int(ocaml_return_code(code));

  value args[] = {ocaml_msg, ocaml_code};

  caml_raise_with_args(*caml_named_value(OCAML_UNQLITE_ERROR), 2, args);
}

inline static void o_raise_simple(const char *msg) {
  o_raise(msg, UNQLITE_UNDEFINED_ERROR);
}

inline static void o_raise_with_library_msg(int rc, unqlite *db, const char *default_msg) {
  const char *msg = NULL;
  int msg_length = 0;

  unqlite_config(db, UNQLITE_CONFIG_ERR_LOG, &msg, &msg_length);
  if( msg_length > 0 )
    o_raise(msg, rc);
  else
    o_raise(default_msg, rc);
}

inline static unqlite* get_db(value db_value) {
  unqlite* db = (unqlite *) Field(db_value, 0);

  if (!db)
    o_raise_simple("Database pointer is corrupt");

  return db;
}

// cast key length of type [mlsize_t] to [int], fail if it would overflow
inline static int determine_unqlite_key_size(value key) {
  mlsize_t s = caml_string_length(key);
  if (s > INT_MAX) {
    o_raise_simple("key too long");
    // keep the compiler happy
    return -1;
  } else {
    return (int) s;
  }
}

// cast data length of type [mlsize_t] to [long long], fail if it would overflow
inline static long long determine_unqlite_data_size(value data) {
  mlsize_t s = caml_string_length(data);
  if (s > LLONG_MAX) {
    o_raise_simple("too much data");
    // keep the compiler happy
    return -1;
  } else {
    return (long long) s;
  }
}

static value o_unqlite_open_internal(value path, unsigned int flags) {
  CAMLparam1(path);
  CAMLlocal1(db_value);

  unqlite *db = NULL;

  /* TODO OCaml strings may contain zero bytes, we should probably fail if
     `strlen != caml_string_length` for security reasons.
   */
  int rc = unqlite_open(&db, String_val(path), flags);
  if (rc != UNQLITE_OK || !db)
    o_raise_with_library_msg(rc, db, "open failed");

  db_value = caml_alloc(1, Abstract_tag);
  Store_field(db_value, 0, (value) db);
  CAMLreturn(db_value);
}

/**********************/
/*     Public API     */
/**********************/

CAMLprim value o_unqlite_open_create(value path) {
  return o_unqlite_open_internal(path, UNQLITE_OPEN_CREATE);
}

CAMLprim value o_unqlite_open_mmap(value path) {
  return o_unqlite_open_internal(path, UNQLITE_OPEN_READONLY | UNQLITE_OPEN_MMAP);
}

CAMLprim value o_unqlite_open_readwrite(value path) {
  return o_unqlite_open_internal(path, UNQLITE_OPEN_READWRITE);
}

CAMLprim value o_unqlite_close(value db_value) {
  CAMLparam1(db_value);
  unqlite *db = get_db(db_value);

  int rc = unqlite_close(db);
  if (rc != UNQLITE_OK)
    o_raise_with_library_msg(rc, db, "close failed");

  CAMLreturn(Val_unit);
}

CAMLprim value o_unqlite_commit(value db_value) {
  CAMLparam1(db_value);
  unqlite *db = get_db(db_value);

  int rc = unqlite_commit(db);
  if (rc != UNQLITE_OK)
    o_raise_with_library_msg(rc, db, "commit failed");

  CAMLreturn(Val_unit);
}

CAMLprim value o_unqlite_rollback(value db_value) {
  CAMLparam1(db_value);

  unqlite *db = get_db(db_value);

  int rc = unqlite_rollback(db);
  if (rc != UNQLITE_OK)
    o_raise_with_library_msg(rc, db, "rollback failed");

  CAMLreturn(Val_unit);
}

CAMLprim value o_unqlite_kv_store(value db_value, value key, value data) {
  CAMLparam3(db_value, key, data);
  unqlite *db = get_db(db_value);

  int rc = unqlite_kv_store(
    db,
    String_val(key), determine_unqlite_key_size(key),
    String_val(data), determine_unqlite_data_size(data));

  if (rc != UNQLITE_OK)
    o_raise_with_library_msg(rc, db, "store failed");

  CAMLreturn(Val_unit);
}

CAMLprim value o_unqlite_kv_append(value db_value, value key, value data) {
  CAMLparam3(db_value, key, data);
  unqlite *db = get_db(db_value);

  int rc = unqlite_kv_append(
    db,
    String_val(key), determine_unqlite_key_size(key),
    String_val(data), determine_unqlite_data_size(data));

  if (rc != UNQLITE_OK)
    o_raise_with_library_msg(rc, db, "append failed");

  CAMLreturn(Val_unit);
}

CAMLprim value o_unqlite_kv_delete(value db_value, value key) {
  CAMLparam2(db_value, key);
  unqlite *db = get_db(db_value);

  int rc = unqlite_kv_delete(
    db, String_val(key), determine_unqlite_key_size(key));

  if (rc == UNQLITE_NOTFOUND)
    caml_raise_not_found();
  if (rc != UNQLITE_OK)
    o_raise_with_library_msg(rc, db, "delete failed");

  CAMLreturn(Val_unit);
}

CAMLprim value o_unqlite_kv_fetch(value db_value, value key_value) {
  CAMLparam2(db_value, key_value);
  CAMLlocal1(data);
  int rc;
  unqlite_int64 size;

  int key_size = determine_unqlite_key_size(key_value);
  const char *key = String_val(key_value);

  unqlite *db = get_db(db_value);

  //Extract data size first
  rc = unqlite_kv_fetch(db, key, key_size, NULL, &size);
  if (rc == UNQLITE_NOTFOUND)
    caml_raise_not_found();
  if (rc != UNQLITE_OK)
    o_raise_with_library_msg(rc, db, "could not get value size");

  //Allocate a buffer big enough to hold the record content
  if (size < 0)
    o_raise_simple("unqlite returned an invalid data size");
  char *buf = (char *) malloc(((size_t) size) + 1);
  if(!buf)
    o_raise_simple("out of memory");

  //Copy record content in our buffer
  rc = unqlite_kv_fetch(db, key, key_size, buf, &size);
  if (rc != UNQLITE_OK)
    o_raise_with_library_msg(rc, db, "could not fetch value");
  // make sure that the string is correctly 0-terminated
  buf[size] = 0;

  /* TODO this is probably sub-optimal! */
  data = caml_copy_string(buf);
  free(buf);

  CAMLreturn(data);
}

/* cursor interface */

inline static unqlite_kv_cursor* get_cursor(value cursor_value) {
  unqlite_kv_cursor* cursor = (unqlite_kv_cursor*) Field(cursor_value, 0);

  if (!cursor)
    o_raise_simple("Cursor pointer is corrupt");

  return cursor;
}

CAMLprim value o_unqlite_cursor_init(value db_value) {
  CAMLparam1(db_value);
  CAMLlocal1(cursor_value);

  unqlite_kv_cursor *cursor = NULL;
  unqlite *db = get_db(db_value);

  int rc = unqlite_kv_cursor_init(db, &cursor);
  if (rc != UNQLITE_OK || !cursor)
    o_raise_with_library_msg(rc, db, "cursor init failed");

  cursor_value = caml_alloc(1, Abstract_tag);
  Store_field(cursor_value, 0, (value) cursor);
  CAMLreturn(cursor_value);
}

CAMLprim value o_unqlite_cursor_release(value db_value, value cursor_value) {
  CAMLparam2(db_value, cursor_value);
  unqlite *db = get_db(db_value);
  unqlite_kv_cursor *cursor = get_cursor(cursor_value);

  int rc = unqlite_kv_cursor_release(db, cursor);
  if (rc != UNQLITE_OK)
    o_raise_with_library_msg(rc, db, "cursor release failed");

  CAMLreturn(Val_unit);
}

CAMLprim value o_unqlite_cursor_first_entry(value cursor_value) {
  CAMLparam1(cursor_value);

  int rc = unqlite_kv_cursor_first_entry(get_cursor(cursor_value));
  if (rc != UNQLITE_OK)
    o_raise("cursor first entry failed", rc);

  CAMLreturn(Val_unit);
}

CAMLprim value o_unqlite_cursor_next_entry(value cursor_value) {
  CAMLparam1(cursor_value);

  int rc = unqlite_kv_cursor_next_entry(get_cursor(cursor_value));
  if (rc != UNQLITE_OK)
    o_raise("cursor next entry failed", rc);

  CAMLreturn(Val_unit);
}

CAMLprim value o_unqlite_cursor_valid_entry(value cursor_value) {
  CAMLparam1(cursor_value);
  CAMLreturn(Val_bool(unqlite_kv_cursor_valid_entry(get_cursor(cursor_value))));
}

CAMLprim value o_unqlite_cursor_key(value cursor_value) {
  CAMLparam1(cursor_value);
  CAMLlocal1(key_value);
  int rc;
  int key_size = -1;

  unqlite_kv_cursor *cursor = get_cursor(cursor_value);

  rc = unqlite_kv_cursor_key(cursor, NULL, &key_size);
  if (rc != UNQLITE_OK)
    o_raise("cursor: could not determine key size", rc);

  if (key_size < 0)
    o_raise_simple("unqlite returned an invalid key size");
  char *buf = (char *) malloc(((size_t) key_size) + 1);
  if(!buf)
    o_raise_simple("out of memory");

  rc = unqlite_kv_cursor_key(cursor, buf, &key_size);
  if (rc != UNQLITE_OK) {
    free(buf);
    o_raise("cursor: could not get key", rc);
  }
  buf[key_size] = 0;

  key_value = caml_copy_string(buf);
  free(buf);

  CAMLreturn(key_value);
}

CAMLprim value o_unqlite_cursor_data(value cursor_value) {
  CAMLparam1(cursor_value);
  CAMLlocal1(data_value);
  int rc;
  unqlite_int64 value_size = -1;

  unqlite_kv_cursor *cursor = get_cursor(cursor_value);

  rc = unqlite_kv_cursor_data(cursor, NULL, &value_size);
  if (rc != UNQLITE_OK)
    o_raise("cursor: could not determine data size", rc);

  if (value_size < 0) o_raise_simple("unqlite returned an invalid data size");
  char *buf = (char *) malloc(((size_t) value_size) + 1);
  if(!buf)
    o_raise_simple("out of memory");

  rc = unqlite_kv_cursor_data(cursor, buf, &value_size);
  if (rc != UNQLITE_OK) {
    free(buf);
    o_raise("cursor: could not get data", rc);
  }
  buf[value_size] = 0;

  data_value = caml_copy_string(buf);
  free(buf);

  CAMLreturn(data_value);
}

/* We need to redefine according to the following sum type:

type error_code =
  |UNQLITE_NOMEM            (* Out of memory *)
  |UNQLITE_ABORT            (* Another thread have released this instance *)
  |UNQLITE_IOERR            (* IO error *)
  |UNQLITE_CORRUPT          (* Corrupt pointer *)
  |UNQLITE_LOCKED           (* Forbidden Operation *)
  |UNQLITE_BUSY            	(* The database file is locked *)
  |UNQLITE_DONE            	(* Operation done *)
  |UNQLITE_PERM             (* Permission error *)
  |UNQLITE_NOTIMPLEMENTED   (* Method not implemented by the underlying Key/Value storage engine *)
  |UNQLITE_NOTFOUND         (* No such record *)
  |UNQLITE_NOOP             (* No such method *)
  |UNQLITE_INVALID          (* Invalid parameter *)
  |UNQLITE_EOF              (* End Of Input *)
  |UNQLITE_UNKNOWN          (* Unknown configuration option *)
  |UNQLITE_LIMIT            (* Database limit reached *)
  |UNQLITE_EXISTS           (* Records exists *)
  |UNQLITE_EMPTY            (* Empty record *)
  |UNQLITE_COMPILE_ERR      (* Compilation error *)
  |UNQLITE_VM_ERR           (* Virtual machine error *)
  |UNQLITE_FULL             (* Full database (unlikely) *)
  |UNQLITE_CANTOPEN         (* Unable to open the database file *)
  |UNQLITE_READ_ONLY        (* Read only Key/Value storage engine *)
  |UNQLITE_LOCKERR          (* Locking protocol error *)

*/
static int ocaml_return_code(int unqlite_return_code) {
  switch (unqlite_return_code) {
    case UNQLITE_NOMEM: return 0;
    case UNQLITE_ABORT: return 1;
    case UNQLITE_IOERR: return 2;
    case UNQLITE_CORRUPT: return 3;
    case UNQLITE_LOCKED: return 4;
    case UNQLITE_BUSY: return 5;
    case UNQLITE_DONE: return 6;
    case UNQLITE_PERM: return 7;
    case UNQLITE_NOTIMPLEMENTED: return 8;
    case UNQLITE_NOTFOUND: return 9;
    case UNQLITE_NOOP: return 10;
    case UNQLITE_INVALID: return 11;
    case UNQLITE_EOF: return 12;
    case UNQLITE_UNKNOWN: return 13;
    case UNQLITE_LIMIT: return 14;
    case UNQLITE_EXISTS: return 15;
    case UNQLITE_EMPTY: return 16;
    case UNQLITE_COMPILE_ERR: return 17;
    case UNQLITE_VM_ERR: return 18;
    case UNQLITE_FULL: return 19;
    case UNQLITE_CANTOPEN: return 20;
    case UNQLITE_READ_ONLY: return 21;
    case UNQLITE_LOCKERR: return 22;
    case UNQLITE_UNDEFINED_ERROR: return OCAML_UNQLITE_ERROR_UNKNOWN;
    default: return OCAML_UNQLITE_ERROR_UNKNOWN;
  }
}
