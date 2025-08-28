#ifndef SQL_COLUMN
#define SQL_COLUMN

#include "SQL_Value.h"
#include <cstdio>

namespace SQL {
struct Column_t {
  unsigned long rowCount = 0;
  SqlValue *values = nullptr;
  Column_t() = default;
  Column_t(unsigned long rowCount) : rowCount(rowCount) {
    values = new SqlValue[rowCount];
  }
  ~Column_t() { destroy(); }

  Column_t(const Column_t &other) { copy_from(other); }
  Column_t &operator=(const Column_t &other) {
    if (this != &other) {
      copy_from(other);
    }
    return *this;
  }

  Column_t(Column_t &&other) noexcept { move_from(std::move(other)); }

  Column_t &operator=(Column_t &&other) {
    if (this != &other) {
      move_from(std::move(other));
    }
    return *this;
  }

  const char *toString() {
    size_t bufSize = 128;
    char *buffer = (char *)malloc(bufSize);
    size_t pos = 0;

    for (size_t r = 0; r < rowCount; ++r) {
      size_t need =
          snprintf(buffer + pos, bufSize - pos, "%s\t", values[r].toString());
      if (need >= bufSize - pos) {
        bufSize *= 2;
        buffer = (char *)realloc(buffer, bufSize);

        need =
            snprintf(buffer + pos, bufSize - pos, "%s\t", values[r].toString());
      }
      pos += need;
    }
    return buffer;
  }

private:
  void destroy() {
    if (values != nullptr) {
      delete[] values;
      values = nullptr;
    }
    rowCount = 0;
  }

  void copy_from(const Column_t &o) {
    destroy();
    rowCount = o.rowCount;
    values = new SqlValue[rowCount];

    for (unsigned long i = 0; i < rowCount; ++i)
      values[i] = o.values[i];
  }

  void move_from(Column_t &&o) noexcept {
    destroy();
    rowCount = o.rowCount;
    values = new SqlValue[rowCount];

    for (unsigned long i = 0; i < rowCount; ++i)
      values[i] = std::move(o.values[i]);

    o.destroy();
  }
};
} // namespace SQL

#endif
