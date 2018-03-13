#include <limits.h>

#include <caml/memory.h>
#include <caml/mlvalues.h>
#include <caml/callback.h>
#include <caml/fail.h>
#include <caml/alloc.h>

#include "unqlite.h"

inline static void o_raise(const char *msg) {
  caml_raise_with_string(*caml_named_value("de.burgerdev.unqlite.error"), msg);
}

inline static unqlite* get_db(value db_value) {
  unqlite* db = (unqlite *) Field(db_value, 0);

  if (!db)
    o_raise("Database has been closed");

  return db;
}

inline static long long to_llong(unsigned long long s, const char *msg) {
  if (s > LLONG_MAX)
    o_raise(msg);
  return (long long) s;
}

inline static int to_int(unsigned long long s, const char *msg) {
  if (s > INT_MAX)
    o_raise(msg);
  return (int) s;
}

extern value o_unqlite_open(value path) {
  CAMLparam1(path);
  CAMLlocal1(db_value);

  int rc;
  unqlite *db;

  /* TODO OCaml strings may contain zero bytes, we should probably fail if
     `strlen != caml_string_length` for security reasons.
   */
  rc = unqlite_open(&db, String_val(path), UNQLITE_OPEN_CREATE);
  if (rc != UNQLITE_OK) o_raise("open failed");

  db_value = caml_alloc(1, Abstract_tag);
  Store_field(db_value, 0, (value) db);
  CAMLreturn(db_value);
}

extern value o_unqlite_close(value db_value) {
  CAMLparam1(db_value);
  unqlite *db = get_db(db_value);

  if (unqlite_close(db) != UNQLITE_OK)
    o_raise("close failed");

  Store_field(db_value, 0, (value) NULL);

  CAMLreturn(Val_unit);
}

extern value o_unqlite_commit(value db_value) {
  CAMLparam1(db_value);
  unqlite *db = get_db(db_value);

  if (unqlite_commit(db) != UNQLITE_OK)
    o_raise("commit failed");

  CAMLreturn(Val_unit);
}

extern value o_unqlite_rollback(value db_value) {
  CAMLparam1(db_value);

  unqlite *db = get_db(db_value);

  if (unqlite_rollback(db) != UNQLITE_OK)
    o_raise("rollback failed");

  CAMLreturn(Val_unit);
}

extern value o_unqlite_kv_store(value db_value, value key, value data) {
  CAMLparam3(db_value, key, data);
  unqlite *db = get_db(db_value);

  int rc = unqlite_kv_store(
    db,
    String_val(key), to_int(caml_string_length(key), "key too long"),
    String_val(data), to_llong(caml_string_length(data), "too much data"));

  if (rc != UNQLITE_OK) o_raise("store failed");

  CAMLreturn(Val_unit);
}

extern value o_unqlite_kv_fetch(value db_value, value key_value) {
  CAMLparam2(db_value, key_value);
  CAMLlocal1(data);
  int rc;
  unqlite_int64 size;

  int key_size = to_int(caml_string_length(key_value), "key too long");
  char *key = String_val(key_value);

  unqlite *db = get_db(db_value);

  //Extract data size first
  rc = unqlite_kv_fetch(db, key, key_size, NULL, &size);
  if (rc == UNQLITE_NOTFOUND) caml_raise_not_found();
  else if (rc != UNQLITE_OK) o_raise("fetch failed (could not get size)");

  //Allocate a buffer big enough to hold the record content
  if (size < 0) o_raise("unqlite returned an invalid data size");
  char *buf = (char *) malloc((size_t) size + 1);
  if(!buf) o_raise("fetch failed (out of memory)");

  //Copy record content in our buffer
  rc = unqlite_kv_fetch(db, key, key_size, buf, &size);
  if (rc != UNQLITE_OK) o_raise("fetch failed (could not fetch data)");
  // make sure that the string is correctly 0-terminated
  buf[size] = 0;

  /* TODO this is probably sub-optimal! */
  data = caml_copy_string(buf);
  free(buf);

  CAMLreturn(data);
}
