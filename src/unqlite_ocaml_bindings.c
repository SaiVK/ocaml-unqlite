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

static const char *OCAML_UNQLITE_ERROR = "de.burgerdev.unqlite.error";

inline static void o_raise(unqlite *db, const char *default_msg) {
  const char *msg = NULL;
  int msg_length = 0;

  unqlite_config(db, UNQLITE_CONFIG_ERR_LOG, &msg, &msg_length);
  if( msg_length > 0 )
    caml_raise_with_string(*caml_named_value(OCAML_UNQLITE_ERROR), msg);
  else
    caml_raise_with_string(*caml_named_value(OCAML_UNQLITE_ERROR), default_msg);
}

inline static unqlite* get_db(value db_value) {
  unqlite* db = (unqlite *) Field(db_value, 0);

  if (!db)
    caml_raise_with_string(
      *caml_named_value(OCAML_UNQLITE_ERROR),
      "Database has been closed");

  return db;
}

// cast key length of type [mlsize_t] to [int], fail if it would overflow
inline static int determine_unqlite_key_size(value key) {
  mlsize_t s = caml_string_length(key);
  if (s > INT_MAX)
    caml_raise_with_string(*caml_named_value(OCAML_UNQLITE_ERROR), "key too long");
  else
    return (int) s;
}

// cast data length of type [mlsize_t] to [long long], fail if it would overflow
inline static long long determine_unqlite_data_size(value data) {
  mlsize_t s = caml_string_length(data);
  if (s > LLONG_MAX)
    caml_raise_with_string(*caml_named_value(OCAML_UNQLITE_ERROR), "too much data");
  else
    return (long long) s;
}

static value o_unqlite_open_internal(value path, unsigned int flags) {
  CAMLparam1(path);
  CAMLlocal1(db_value);

  unqlite *db = NULL;

  /* TODO OCaml strings may contain zero bytes, we should probably fail if
     `strlen != caml_string_length` for security reasons.
   */
  if (unqlite_open(&db, String_val(path), flags) != UNQLITE_OK || !db)
    o_raise(db, "open failed");

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

  if (unqlite_close(db) != UNQLITE_OK)
    o_raise(db, "close failed");

  CAMLreturn(Val_unit);
}

CAMLprim value o_unqlite_commit(value db_value) {
  CAMLparam1(db_value);
  unqlite *db = get_db(db_value);

  if (unqlite_commit(db) != UNQLITE_OK)
    o_raise(db, "commit failed");

  CAMLreturn(Val_unit);
}

CAMLprim value o_unqlite_rollback(value db_value) {
  CAMLparam1(db_value);

  unqlite *db = get_db(db_value);

  if (unqlite_rollback(db) != UNQLITE_OK)
    o_raise(db, "rollback failed");

  CAMLreturn(Val_unit);
}

CAMLprim value o_unqlite_kv_store(value db_value, value key, value data) {
  CAMLparam3(db_value, key, data);
  unqlite *db = get_db(db_value);

  int rc = unqlite_kv_store(
    db,
    String_val(key), determine_unqlite_key_size(key),
    String_val(data), determine_unqlite_data_size(data));

  if (rc != UNQLITE_OK) o_raise(db, "store failed");

  CAMLreturn(Val_unit);
}

CAMLprim value o_unqlite_kv_append(value db_value, value key, value data) {
  CAMLparam3(db_value, key, data);
  unqlite *db = get_db(db_value);

  int rc = unqlite_kv_append(
    db,
    String_val(key), determine_unqlite_key_size(key),
    String_val(data), determine_unqlite_data_size(data));

  if (rc != UNQLITE_OK) o_raise(db, "append failed");

  CAMLreturn(Val_unit);
}

CAMLprim value o_unqlite_kv_delete(value db_value, value key) {
  CAMLparam2(db_value, key);
  unqlite *db = get_db(db_value);

  int rc = unqlite_kv_delete(
    db, String_val(key), determine_unqlite_key_size(key));

  if (rc == UNQLITE_NOTFOUND) caml_raise_not_found();
  if (rc != UNQLITE_OK) o_raise(db, "delete failed");

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
  if (rc == UNQLITE_NOTFOUND) caml_raise_not_found();
  else if (rc != UNQLITE_OK) o_raise(db, "could not get value size");

  //Allocate a buffer big enough to hold the record content
  if (size < 0) o_raise(db, "unqlite returned an invalid data size");
  char *buf = (char *) malloc(((size_t) size) + 1);
  if(!buf) caml_raise_with_string(*caml_named_value(OCAML_UNQLITE_ERROR), "out of memory");

  //Copy record content in our buffer
  rc = unqlite_kv_fetch(db, key, key_size, buf, &size);
  if (rc != UNQLITE_OK) o_raise(db, "could not fetch value");
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
    caml_raise_with_string(
      *caml_named_value(OCAML_UNQLITE_ERROR),
      "invalid cursor");

  return cursor;
}

inline static void o_raise_simple(const char *msg) {
  caml_raise_with_string(*caml_named_value(OCAML_UNQLITE_ERROR), msg);
}

CAMLprim value o_unqlite_cursor_init(value db_value) {
  CAMLparam1(db_value);
  CAMLlocal1(cursor_value);

  unqlite_kv_cursor *cursor = NULL;
  unqlite *db = get_db(db_value);

  if (unqlite_kv_cursor_init(db, &cursor) != UNQLITE_OK || !cursor)
    o_raise(db, "cursor init failed");

  cursor_value = caml_alloc(1, Abstract_tag);
  Store_field(cursor_value, 0, (value) cursor);
  CAMLreturn(cursor_value);
}

CAMLprim value o_unqlite_cursor_release(value db_value, value cursor_value) {
  CAMLparam2(db_value, cursor_value);
  unqlite *db = get_db(db_value);
  unqlite_kv_cursor *cursor = get_cursor(cursor_value);

  if (unqlite_kv_cursor_release(db, cursor) != UNQLITE_OK)
    o_raise(db, "cursor release failed");

  CAMLreturn(Val_unit);
}

CAMLprim value o_unqlite_cursor_first_entry(value cursor_value) {
  CAMLparam1(cursor_value);
  unqlite_kv_cursor *cursor = get_cursor(cursor_value);

  if (unqlite_kv_cursor_first_entry(cursor) != UNQLITE_OK)
    o_raise_simple("cursor first entry failed");

  CAMLreturn(Val_unit);
}

CAMLprim value o_unqlite_cursor_next_entry(value cursor_value) {
  CAMLparam1(cursor_value);
  unqlite_kv_cursor *cursor = get_cursor(cursor_value);

  if (unqlite_kv_cursor_next_entry(cursor) != UNQLITE_OK)
    o_raise_simple("cursor next entry failed");

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
  if (rc != UNQLITE_OK) o_raise_simple("cursor: could not determine key size");

  if (key_size < 0) o_raise_simple("unqlite returned an invalid key size");
  char *buf = (char *) malloc(((size_t) key_size) + 1);
  if(!buf) caml_raise_with_string(*caml_named_value(OCAML_UNQLITE_ERROR), "out of memory");

  rc = unqlite_kv_cursor_key(cursor, buf, &key_size);
  if (rc != UNQLITE_OK) {
    free(buf);
    o_raise_simple("cursor: could not get key");
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
  if (rc != UNQLITE_OK) o_raise_simple("cursor: could not determine data size");

  if (value_size < 0) o_raise_simple("unqlite returned an invalid data size");
  char *buf = (char *) malloc(((size_t) value_size) + 1);
  if(!buf) caml_raise_with_string(*caml_named_value(OCAML_UNQLITE_ERROR), "out of memory");

  rc = unqlite_kv_cursor_data(cursor, buf, &value_size);
  if (rc != UNQLITE_OK) {
    free(buf);
    o_raise_simple("cursor: could not get data");
  }
  buf[value_size] = 0;

  data_value = caml_copy_string(buf);
  free(buf);

  CAMLreturn(data_value);
}

/*
int unqlite_kv_cursor_key(unqlite_kv_cursor *pCursor,void *pBuf,int *pnByte);
int unqlite_kv_cursor_data(unqlite_kv_cursor *pCursor,void *pBuf,unqlite_int64 *pnData);
*/
