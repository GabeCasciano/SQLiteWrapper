#ifndef SQL_ROW
#define SQL_ROW

#include "SQL_Value.h"
#include <cstdio>

namespace SQL {
struct Row_t {
  size_t colCount = 0;
  SqlValue *values = nullptr;
  Row_t() = default;
  Row_t(size_t colCount) : colCount(colCount) {
    this->values = new SqlValue[colCount];
  }
  ~Row_t() { destroy(); }

  Row_t(const Row_t &other) { copy_from(other); }
  Row_t &operator=(const Row_t &other) {
    if (this != &other)
      copy_from(other);
    return *this;
  }
  Row_t(Row_t &&other) noexcept { move_from(std::move(other)); }
  Row_t &operator=(Row_t &&other) noexcept {
    if (this != &other)
      move_from(std::move(other));
    return *this;
  }

  void insertValue(SqlValue value, size_t cIdx) {
    if (cIdx >= colCount)
      return;
    if (values != nullptr)
      values[cIdx] = value;
  }

  const char *toSQLString() {
    size_t bufSize = (MAX_COLUMN_NAME_LENGTH + 1) * colCount + 1;

    char *buffer = (char *)malloc(bufSize);

    const char *fmt_str = "%s,";
    const char *last_fmt_str = "%s";

    for (size_t c = 0; c < colCount; ++c)
      sprintf(buffer + (c * MAX_COLUMN_NAME_LENGTH),
              (c == colCount - 1) ? fmt_str : last_fmt_str,
              values[c].toString());

    return buffer;
  }

  const char *toString() {
    size_t bufSize = 128;
    char *buffer = (char *)malloc(bufSize);
    size_t pos = 0;

    for (size_t c = 0; c < colCount; ++c) {
      size_t need =
          snprintf(buffer + pos, bufSize - pos, "%s\t", values[c].toString());
      if (need >= bufSize - pos) {
        bufSize *= 2;
        buffer = (char *)realloc(buffer, bufSize);

        need =
            snprintf(buffer + pos, bufSize - pos, "%s\t", values[c].toString());
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
    colCount = 0;
  }

  void copy_from(const Row_t &o) {
    destroy();
    this->colCount = o.colCount;
    this->values = new SqlValue[colCount];

    for (size_t i = 0; i < colCount; ++i)
      values[i] = o.values[i];
  }

  void move_from(Row_t &&o) noexcept {
    destroy();
    this->colCount = o.colCount;
    this->values = new SqlValue[colCount];
    for (size_t i = 0; i < colCount; ++i)
      values[i] = std::move(o.values[i]);
    o.destroy();
  }
};
} // namespace SQL

#endif // !SQL_ROW
