#ifndef SQL_DATATYPES_H_
#define SQL_DATATYPES_H_

#include "SQL_Utility.h"
#include "SQL_Value.h"

#define MAX_COLUMN_NAME_LENGTH (32)

struct Column_t {
  char name[MAX_COLUMN_NAME_LENGTH];
  SqlValue value;
  bool primaryKey = false;
  Column_t(const char *name, SqlValue value, bool primaryKey)
      : value(value), primaryKey(primaryKey) {
    safeNameCopy((char *)&this->name, name, MAX_COLUMN_NAME_LENGTH);
  }
  Column_t() = default;
};

struct Row_t {
  short colCount;
  Column_t *columns;
  Row_t(short colCount)
      : colCount(colCount),
        columns((Column_t *)malloc(sizeof(Column_t) * colCount)) {}
  Row_t(Column_t *columns, short colCount) : colCount(colCount) {
    this->columns = new Column_t[colCount];
    memcpy((void *)this->columns, columns, sizeof(Column_t) * colCount);
  }
  Row_t() = default;
  ~Row_t() {
    if (!columns)
      free(columns);
  }
};

struct ColumnOfData {
  long rowCount;
  char name[MAX_COLUMN_NAME_LENGTH];
  SqlValue *values;

  ColumnOfData(const char *name, long rowCount)
      : rowCount(rowCount),
        values((SqlValue *)malloc(sizeof(SqlValue) * rowCount)) {
    safeNameCopy((char *)&this->name, name, MAX_COLUMN_NAME_LENGTH);
  }

  ColumnOfData(const char *name, SqlValue *values, long rowCount)
      : rowCount(rowCount),
        values((SqlValue *)malloc(sizeof(SqlValue) * rowCount)) {
    safeNameCopy((char *)&this->name, name, MAX_COLUMN_NAME_LENGTH);
    memcpy((void *)this->values, values, sizeof(SqlValue) * rowCount);
  }

  ~ColumnOfData() {
    if (values)
      free(values);
  }
};

struct Table {
  short colCount;
  long rowCount = 0;
  long rowCap = 0;
  char **names;
  SqlValue **values;

  Table(short colCount)
      : colCount(colCount), names((char **)malloc(sizeof(char) * colCount *
                                                  MAX_COLUMN_NAME_LENGTH)) {}
  Table(short colCount, char **names)
      : colCount(colCount), names((char **)malloc(sizeof(char) * colCount *
                                                  MAX_COLUMN_NAME_LENGTH)) {
    for (int i = 0; i < colCount; ++i)
      safeNameCopy((char *)&this->names[i], names[i], MAX_COLUMN_NAME_LENGTH);
  }
  Table(Row_t row)
      : colCount(row.colCount), names((char **)malloc(sizeof(char) * colCount *
                                                      MAX_COLUMN_NAME_LENGTH)) {
    for (int i = 0; i < colCount; ++i)
      safeNameCopy((char *)&this->names[i], row.columns[i].name,
                   MAX_COLUMN_NAME_LENGTH);
  }
  Table() = default;
  ~Table() {
    if (names)
      free(names);
    if (values)
      free(values);
  }

  void setColName(const char *name, short colNum) {
    if (colNum < colCount)
      safeNameCopy(names[colNum], name, MAX_COLUMN_NAME_LENGTH);
  }

  bool appendRow(SqlValue *values) {
    if (rowCount >= rowCap) {
      rowCap = (rowCap) ? rowCap * 2 : 1;
      SqlValue **tmp = (SqlValue **)realloc(
          (void *)values, sizeof(SqlValue) * rowCap * colCount);
      if (!tmp)
        return false;
      this->values = tmp;
    }
    memcpy((void *)this->values[rowCount], values, colCount * sizeof(SqlValue));
    rowCount++;
    return true;
  }
};

#endif
