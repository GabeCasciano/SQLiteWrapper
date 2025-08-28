#ifndef SQL_DATATYPES_H_
#define SQL_DATATYPES_H_

#include "SQL_Column.h"
#include "SQL_Row.h"
#include "SQL_Value.h"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sched.h>

namespace SQL {

struct Matrix_t {

  size_t colCount = 0;
  size_t rowCount = 0;
  char name[MAX_TABLE_NAME_LENGTH];
  SqlValue *values = nullptr;
  char *columnNames = nullptr;

  // default constructor
  Matrix_t() = default;
  // construcotr
  Matrix_t(const char *name, const size_t colCount, const size_t rowCount,
           const char *columnNames) {

    snprintf(this->name, MAX_TABLE_NAME_LENGTH, "%s", name);
    create(colCount, rowCount);
    copy_names((char *)columnNames, colCount);
  }
  Matrix_t(const char *name, const size_t colCount, const char *columnNames) {
    snprintf(this->name, MAX_TABLE_NAME_LENGTH, "%s", name);
    create(colCount, 1);
    copy_names((char *)columnNames, colCount);
  }
  Matrix_t(const size_t colCount, const char *columnNames) {
    create(colCount, 1);
    copy_names((char *)columnNames, colCount);
  }
  Matrix_t(const size_t colCount) { create(colCount, capacity); }
  Matrix_t(const size_t colCount, const size_t rowCount) {
    create(colCount, rowCount);
  }
  Matrix_t(const char *name, const size_t colCount) {
    snprintf(this->name, MAX_TABLE_NAME_LENGTH, "%s", name);
    create(colCount, 1);
  }
  // destructor
  ~Matrix_t() { destroy(); }

  // copy
  Matrix_t(const Matrix_t &other) { copy_from(other); }
  // copy assignment
  Matrix_t &operator=(const Matrix_t &other) {
    if (this != &other) {
      // destroy(); // Copy does the destroy
      copy_from(other);
    }
    return *this;
  }
  // move
  Matrix_t(Matrix_t &&other) noexcept { move_from(std::move(other)); }
  // move asignment
  Matrix_t &operator=(Matrix_t &&other) noexcept {
    if (this != &other) {
      // destroy(); // move does the destroy
      move_from(std::move(other));
    }
    return *this;
  }

  Row_t getRow(size_t rIdx) {
    if (rIdx >= rowCount || values == nullptr)
      return Row_t();

    Row_t r = Row_t(colCount);

    SqlValue *cpy_ptr = values;
    cpy_ptr += rIdx * colCount;
    for (size_t i = 0; i < colCount; ++i)
      r.values[i] = *(cpy_ptr + i);

    free(cpy_ptr);
    return r;
  }

  void appendRow(Row_t r) {
    if (r.colCount != colCount)
      return;

    if (rowCount >= capacity) {
      Matrix_t cpy = *this;
      cpy.capacity *= 2;
      copy_from(cpy);
    }

    SqlValue *write_ptr = values;
    for (size_t i = 0; i < colCount; ++i)
      values[rowCount * colCount + i] = r.values[i];

    rowCount++;
  }

  Column_t getColumn(size_t cIdx) {
    if (cIdx >= colCount || values == nullptr)
      return Column_t();

    Column_t c = Column_t(rowCount);

    SqlValue *cpy_ptr = values;
    cpy_ptr += cIdx;
    for (size_t i = 0; i < rowCount; ++i)
      c.values[i] = *(cpy_ptr + (i * colCount));

    return c;
  }

  const char *getColumnName(size_t cIdx) {
    if (cIdx >= colCount)
      return "";

    return columnNames + (cIdx * MAX_COLUMN_NAME_LENGTH);
  }

  void setColumnName(const char *colName, size_t cIdx) {
    if (cIdx >= colCount)
      return;

    strcpy(columnNames + (cIdx * MAX_COLUMN_NAME_LENGTH), colName);
  }

  const char *getSQLColumnNamesString() {
    size_t bufSize = (MAX_COLUMN_NAME_LENGTH + 1) * colCount + 1;
    char *buffer = (char *)malloc(bufSize);

    const char *fmt_str = "%s,";
    const char *last_fmt_str = "%s";

    for (size_t c = 0; c < colCount; ++c)
      sprintf(buffer + (c * MAX_COLUMN_NAME_LENGTH),
              (c == colCount - 1) ? fmt_str : last_fmt_str, getColumnName(c));

    return buffer;
  }

  const char *toString() {
    size_t bufSize = 128;
    char *buffer = (char *)malloc(bufSize);

    size_t pos = 0;
    for (size_t r = 0; r < rowCount; ++r) {
      size_t need =
          snprintf(buffer + pos, bufSize - pos, "%s\n", getRow(r).toString());

      if (need >= bufSize - pos) {
        bufSize *= 2;
        buffer = (char *)realloc(buffer, bufSize);

        need =
            snprintf(buffer + pos, bufSize - pos, "%s\n", getRow(r).toString());
      }
      pos += need;
    }

    return buffer;
  }

private:
  size_t capacity = 1;

  void create(size_t colCount, size_t capacity) {
    this->colCount = colCount;
    this->values = new SqlValue[capacity * colCount];
    this->columnNames = new char[colCount * MAX_COLUMN_NAME_LENGTH];
  }

  void copy_names(char *columnNames, size_t count) {
    if (count != colCount)
      return;

    strcpy(this->columnNames, columnNames);
  }

  void destroy() {
    if (values != nullptr) {
      delete[] values;
      values = nullptr;
    }
    if (columnNames != nullptr) {
      delete[] columnNames;
      columnNames = nullptr;
    }
    sprintf(name, "");
    colCount = 0;
    rowCount = 0;
    capacity = 0;
  }

  void copy_from(const Matrix_t &o) {
    destroy();
    rowCount = o.rowCount;
    colCount = o.colCount;
    capacity = o.capacity;
    strcpy(name, o.name);
    create(o.colCount, o.capacity);
    memcpy((void *)values, o.values, sizeof(SqlValue) * colCount * rowCount);
    copy_names(o.columnNames, o.colCount);
  }

  void move_from(Matrix_t &&o) noexcept {
    destroy();
    rowCount = o.rowCount;
    colCount = o.colCount;
    capacity = o.capacity;
    strcpy(name, o.name);
    create(o.colCount, o.capacity);
    values = std::move(o.values);
    columnNames = std::move(o.columnNames);
    o.destroy();
  }
};
} // namespace SQL

#endif
