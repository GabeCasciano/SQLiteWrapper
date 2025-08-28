#ifndef SQL_DATATYPES_H_
#define SQL_DATATYPES_H_

#include "SQL_Column.h"
#include "SQL_Row.h"
#include "SQL_Value.h"

#ifndef ARDUINO
#include <cstdio>
#include <cstring>
#include <format>
#endif

namespace SQL {

struct Matrix_t {

  unsigned short colCount = 0;
  unsigned long rowCount = 0;
  char name[MAX_TABLE_NAME_LENGTH];
  SqlValue *values = nullptr;
  char *columnNames = nullptr;

  // default constructor
  Matrix_t() = default;
  // construcotr
  Matrix_t(const char *name, const unsigned short colCount,
           const unsigned long rowCount, const char **columnNames)
      : colCount(colCount), rowCount(rowCount), capacity(rowCount) {

    snprintf(this->name, MAX_TABLE_NAME_LENGTH, "%s", name);
    create(colCount, rowCount);
    copy_names((char **)columnNames, colCount);
  }
  Matrix_t(const char *name, const unsigned short colCount,
           const char **columnNames)
      : colCount(colCount) {
    snprintf(this->name, MAX_TABLE_NAME_LENGTH, "%s", name);
    create(colCount, capacity);
    copy_names((char **)columnNames, colCount);
  }
  Matrix_t(const unsigned short colCount, const char **columnNames)
      : colCount(colCount) {
    create(colCount, capacity);
    copy_names((char **)columnNames, colCount);
  }
  Matrix_t(const unsigned short colCount) { create(colCount, capacity); }
  Matrix_t(const unsigned short colCount, const unsigned long rowCount)
      : colCount(colCount), rowCount(rowCount), capacity(rowCount) {
    create(colCount, rowCount);
  }
  Matrix_t(const char *name, const unsigned short colCount)
      : colCount(colCount) {
    snprintf(this->name, MAX_TABLE_NAME_LENGTH, "%s", name);
    create(colCount, capacity);
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

  Row_t getRow(unsigned long rIdx) {
    if (rIdx >= rowCount || values == nullptr)
      return Row_t();

    Row_t r = Row_t(colCount);

    SqlValue *cpy_ptr = values;
    cpy_ptr += rIdx * colCount;
    for (unsigned short i = 0; i < colCount; ++i)
      r.values[i] = *(cpy_ptr + i);

    free(cpy_ptr);
    return r;
  }

  Column_t getColumn(unsigned short cIdx) {
    if (cIdx >= colCount || values == nullptr)
      return Column_t();

    Column_t c = Column_t(rowCount);

    SqlValue *cpy_ptr = values;
    cpy_ptr += cIdx;
    for (unsigned long i = 0; i < rowCount; ++i)
      c.values[i] = *(cpy_ptr + (i * colCount));

    return c;
  }

  const char *getColumnName(unsigned short cIdx) {
    if (cIdx >= colCount)
      return "";

    return columnNames + (cIdx * MAX_COLUMN_NAME_LENGTH);
  }

  void appendRow(Row_t r) {
    if (r.colCount != colCount)
      return;

    if (rowCount >= capacity) {
      capacity *= 2;

      Matrix_t cpy = *this;

      destroy();

      create(cpy.colCount, cpy.capacity);
      capacity = cpy.capacity;

      cpy.destroy();
    }

    for (unsigned short i = 0; i < colCount; ++i)
      values[i][rowCount] = r.values[i];

    rowCount++;
  }

  const char *toString() {}

private:
  unsigned long capacity = 1;
  const char *toStr;

  void create(unsigned short colCount, unsigned long rowCount) {
    this->colCount = colCount;
    this->rowCount = rowCount;
    this->values = new SqlValue[rowCount * colCount];
    this->columnNames = new char[colCount * MAX_COLUMN_NAME_LENGTH];
  }

  void copy_names(char *columnNames, unsigned short count) {
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
  }

  void copy_from(const Matrix_t &o) {
    destroy();
    rowCount = o.rowCount;
    colCount = o.colCount;
    capacity = o.capacity;
    strcpy(name, o.name);
    create(o.colCount, o.rowCount);
    memcpy((void *)values, o.values, sizeof(SqlValue) * colCount * rowCount);
    copy_names(o.columnNames, o.colCount);
  }

  void move_from(Matrix_t &&o) noexcept {
    destroy();
    rowCount = o.rowCount;
    colCount = o.colCount;
    capacity = o.capacity;
    strcpy(name, o.name);
    create(o.colCount, o.rowCount);
    values = std::move(o.values);
    columnNames = std::move(o.columnNames);
    o.destroy();
  }
};
} // namespace SQL

#endif
