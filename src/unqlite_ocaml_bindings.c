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

  Store_field(db_value, 0, (value) NULL);

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
