#ifndef SQL_VALUE_H
#define SQL_VALUE_H

#include <assert.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sqlite3.h>
#include <utility>

namespace SQL {

#define MAX_COLUMN_NAME_LENGTH (32)
#define MAX_TABLE_NAME_LENGTH (32)

struct SqlValue {
  enum Type { Null = 0, Integer = 1, Real = 2, Text = 3, Blob = 4 };

  // default
  SqlValue() : kind(Type::Null) {}
  // constructors
  SqlValue(long v) : kind(Type::Integer) { st.i = v; }
  SqlValue(double v) : kind(Type::Real) { st.r = v; }
  SqlValue(const char *s) : kind(Type::Text) {
    if (!s) {
      kind = Type::Null;
      size = 0;
      st.s = nullptr;
      return;
    }
    size = strlen(s);
    st.s = new char[size];
    strcpy(st.s, s);
  }
  SqlValue(const void *data, size_t n) : kind(Type::Blob), size(n) {
    st.b = new uint8_t[size];
    memcpy(st.b, data, size);
  }

  // Copy
  SqlValue(const SqlValue &other) : kind(Type::Null) { copy_from(other); }
  // copy assignment
  SqlValue &operator=(const SqlValue &other) {
    if (this != &other) {
      destroy();
      copy_from(other);
    }
    return *this;
  }

  // Move
  SqlValue(SqlValue &&other) noexcept : kind(Type::Null) {
    move_from(std::move(other));
  }
  // Move assignment
  SqlValue &operator=(SqlValue &&other) noexcept {
    if (this != &other) {
      destroy();
      move_from(std::move(other));
    }
    return *this;
  }

  ~SqlValue() { destroy(); }

  // Equality
  bool operator==(const SqlValue &other) const {
    if (kind != other.kind)
      return false;

    switch (kind) {
    case Null:
      return true;
    case Integer:
      return st.i == other.st.i;
    case Real:
      return st.r == other.st.r;
    case Text:
      return std::strcmp(st.s, other.st.s) == 0;
    case Blob:
      return size == other.size && std::memcmp(st.b, other.st.b, size) == 0;
    }

    return false; // unreachable
  }
  bool operator!=(const SqlValue &other) const { return !(*this == other); }

  // Ordering (only for numeric values)
  bool operator<(const SqlValue &other) const {
    if (kind == Null)
      return other.kind != Null;
    if (other.kind == Null)
      return false;

    if (kind == Integer && other.kind == Integer)
      return st.i < other.st.i;
    if (kind == Real && other.kind == Real)
      return st.r < other.st.r;
    if (kind == Integer && other.kind == Real)
      return (double)st.i < other.st.r;
    if (kind == Real && other.kind == Integer)
      return st.r < (double)other.st.i;

    if (kind == Text && other.kind == Text)
      return strcmp(st.s, other.st.s) < 0;
    if (kind == Blob && other.kind == Blob) {
      const int cmp = memcmp(st.b, other.st.b, std::min(size, other.size));
      return (cmp < 0) || (cmp == 0 && size < other.size);
    }
    return false;
  }

  bool operator>(const SqlValue &other) const { return other < *this; }

  bool operator<=(const SqlValue &other) const { return !(other < *this); }

  bool operator>=(const SqlValue &other) const { return !(*this < other); }

  long type() { return kind; }

  const char *typeString() {
    switch (kind) {
    case Type::Null:
      return "NULL\0";
    case Type::Integer:
      return "INTEGER\0";
    case Type::Real:
      return "REAL\0";
    case Type::Text:
      return "TEXT\0";
    case Type::Blob:
      return "BLOB\0";
    default:
      return "NULL\0";
    }
  }

  const char *toString() {
    size_t bufSize = 32;
    char *buffer = (char *)malloc(bufSize);

    switch (kind) {
    case Type::Null:
      sprintf(buffer, "NULL");
      break;
    case Type::Integer:
      sprintf(buffer, "%ld", st.i);
      break;
    case Type::Real:
      sprintf(buffer, "%.3lf", st.r);
      break;
    case Type::Text:
    case Type::Blob:
      size_t need = snprintf(NULL, 0, "'%s'", st.s);
      buffer = (char *)realloc(buffer, need);
      strcpy(buffer, st.s);
      break;
    }
    return buffer;
  }

  // Accessors (assert on wrong' type for simplicity)
  long as_int() {
    assert(kind == Type::Integer);
    return st.i;
  }
  double as_real() {
    assert(kind == Type::Real);
    return st.r;
  }
  const char *as_text() const { return (const char *)st.s; }
  const uint8_t *as_blob() const { return (uint8_t *)st.b; }

  // Helpers to create from sqlite3 column
  static SqlValue from_column(sqlite3_stmt *stmt, int col) {
    int t = sqlite3_column_type(stmt, col);
    switch (t) {
    case SQLITE_NULL:
      return SqlValue{};
    case SQLITE_INTEGER:
      return SqlValue(static_cast<long>(sqlite3_column_int64(stmt, col)));
    case SQLITE_FLOAT:
      return SqlValue(sqlite3_column_double(stmt, col));
    case SQLITE_TEXT: {
      const unsigned char *p = sqlite3_column_text(stmt, col);
      int n = sqlite3_column_bytes(stmt, col);
      return SqlValue(p, n);
    }
    case SQLITE_BLOB: {
      const void *p = sqlite3_column_blob(stmt, col);
      int n = sqlite3_column_bytes(stmt, col);
      return SqlValue(p, n);
    }
    default:
      return SqlValue{}; // defensive
    }
  }

private:
  Type kind;
  size_t size; // in bytes

  union Storage {
    long i;
    double r;
    char *s;    // non-trivial -> placement new + manual dtor
    uint8_t *b; // non-trivial
    Storage() {}
    ~Storage() {}
  } st;

  void destroy() {
    switch (kind) {
    case Type::Text:
      delete[] st.s;
      break;
    case Type::Blob:
      delete[] st.b;
      break;
    }
    kind = Type::Null;
    size = 0;
  }

  void copy_from(const SqlValue &o) {
    size = o.size;
    kind = o.kind;
    switch (o.kind) {
    case Type::Null:
      break;
    case Type::Integer:
      st.i = o.st.i;
      break;
    case Type::Real:
      st.r = o.st.r;
      break;
    case Type::Text:
      st.s = new char[size];
      strcpy(st.s, o.st.s);
      break;
    case Type::Blob:
      st.b = new uint8_t[size];
      memcpy((void *)&st.b, o.st.b, size);
      break;
    }
  }

  void move_from(SqlValue &&o) noexcept {
    kind = o.kind;
    size = o.size;
    switch (o.kind) {
    case Type::Null:
      break;
    case Type::Integer:
      st.i = o.st.i;
      break;
    case Type::Real:
      st.r = o.st.r;
      break;
    case Type::Text:
      st.s = new char[size];
      st.s = std::move(o.st.s);
      break;
    case Type::Blob:
      st.b = new uint8_t[size];
      st.b = std::move(o.st.b);
      break;
    }
    o.destroy();
  }
};
} // namespace SQL

#endif
