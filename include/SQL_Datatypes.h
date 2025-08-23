#ifndef SQL_DATATYPES_H_
#define SQL_DATATYPES_H_

#include "SQL_Utility.h"
#include "SQL_Value.h"
#include <cstdlib>
#include <cstring>

#define MAX_COLUMN_NAME_LENGTH (32)
#define MAX_TABLE_NAME_LENGTH (32)

struct Row_t {
  unsigned short colCount = 0;
  SqlValue *values;
  Row_t() = default;
  Row_t(unsigned short colCount) : colCount(colCount) {
    this->values = new SqlValue[colCount];
  }
  Row_t(unsigned short colCount, unsigned long rIdx, SqlValue **src)
      : colCount(colCount) {
    this->values = new SqlValue[colCount];
    copy_data(src, rIdx);
  }
  ~Row_t() {
    if (values)
      delete[] values;
    colCount = 0;
  }

private:
  void copy_data(SqlValue **src, unsigned long rIdx) {
    if (!values)
      return;

    for (unsigned i = 0; i < colCount; ++i)
      values[i] = src[i][rIdx];
  }
};

struct Column_t {
  unsigned long rowCount = 0;
  SqlValue *values;
  Column_t() = default;
  Column_t(unsigned long rowCount) : rowCount(rowCount) {
    values = new SqlValue[rowCount];
  }
  Column_t(unsigned long rowCount, SqlValue *src) : rowCount(rowCount) {
    values = new SqlValue[rowCount];
    copy_data(src);
  }
  ~Column_t() {
    if (values)
      delete[] values;
  }

private:
  void copy_data(SqlValue *src) {
    if (!values)
      return;
    memcpy((void *)values, src, rowCount);
  }
};

struct Matrix_t {

  unsigned short colCount = 0;
  unsigned long rowCount = 0;
  char name[MAX_TABLE_NAME_LENGTH];
  SqlValue **values;
  char **columnNames;

  // default constructor
  Matrix_t() = default;
  // construcotr
  Matrix_t(const char *name, const unsigned short colCount,
           const unsigned long rowCount, const char **columnNames)
      : colCount(colCount), rowCount(rowCount) {

    safeNameCopy(this->name, name, MAX_TABLE_NAME_LENGTH);
    create(colCount, rowCount);
    copy_names((char **)columnNames, colCount);
  }
  Matrix_t(const char *name, const unsigned short colCount,
           const char **columnNames)
      : colCount(colCount) {
    safeNameCopy(this->name, name, MAX_TABLE_NAME_LENGTH);
    create(colCount, capacity);
    copy_names((char **)columnNames, colCount);
  }
  Matrix_t(const unsigned short colCount, const char **columnNames)
      : colCount(colCount) {
    create(colCount, capacity);
    copy_names((char **)columnNames, colCount);
  }
  Matrix_t(const unsigned short colCount) { create(colCount, capacity); }
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
    if (rIdx >= rowCount)
      return Row_t();

    return Row_t(colCount, rIdx, values);
  }

  Column_t getColumn(unsigned short cIdx) {
    if (cIdx >= colCount)
      return Column_t();

    return Column_t(rowCount, values[cIdx]);
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

private:
  unsigned long capacity = 1;

  void create(unsigned short colCount, unsigned long rowCount) {
    this->columnNames = new char *[colCount];
    this->values = new SqlValue *[colCount];
    for (unsigned short i = 0; i < colCount; ++i) {
      this->columnNames[i] = new char[MAX_COLUMN_NAME_LENGTH];
      this->values[i] = new SqlValue[rowCount];
    }
  }

  void copy_names(char **columnNames, unsigned short count) {
    if (count != colCount)
      return;

    for (unsigned short i = 0; i < count; ++i)
      safeNameCopy(this->columnNames[i], columnNames[i],
                   MAX_COLUMN_NAME_LENGTH);
  }

  void destroy() {
    if (values)
      delete[] values;
    if (columnNames)
      delete[] columnNames;
    safeNameCopy(name, "", MAX_COLUMN_NAME_LENGTH);
    colCount = 0;
    rowCount = 0;
  }

  void copy_from(const Matrix_t &o) {
    destroy();
    safeNameCopy(name, o.name, MAX_COLUMN_NAME_LENGTH);
    create(o.colCount, o.rowCount);
    memcpy((void *)values, o.values, sizeof(SqlValue) * colCount * rowCount);
    copy_names(o.columnNames, o.colCount);
  }

  void move_from(Matrix_t &&o) noexcept {
    destroy();
    safeNameCopy(name, o.name, MAX_COLUMN_NAME_LENGTH);
    create(o.colCount, o.rowCount);
    values = std::move(o.values);
    columnNames = std::move(o.columnNames);
    o.destroy();
  }
};

#endif
