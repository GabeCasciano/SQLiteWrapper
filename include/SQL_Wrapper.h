#ifndef SQL_DB_H
#define SQL_DB_H

#include "SQL_Matrix.h"
#include "SQL_Value.h"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifndef ARDUINO
#include <stdexcept>
#endif

namespace SQL {

class SQL_DB {

public:
  SQL_DB(const char *filename) : filename(filename) {
    if (sqlite3_open_v2(filename, &db,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                        nullptr) != SQLITE_OK)
      return;

    sql_err = nullptr;
  }

  ~SQL_DB() {
    sqlite3_close_v2(db);
    if (sql_err != nullptr)
      sqlite3_free(sql_err);
  }

  inline bool tableExists(const char *tableName) {
    // This bool and call back are used to check if the table exists in the db
    bool exists = false;
    auto callback = [](void *data, int argc, char **argv,
                       char **colNames) -> int {
      bool *exists_ptr = (bool *)(data);
      *exists_ptr = (argc > 0 && argv[0] != nullptr);
      return false;
    };

    const char *fmt_str =
        "SELECT name FROM sqlite_master WHERE type='table' AND name='%s';";

    size_t need = snprintf(NULL, 0, fmt_str, tableName);
    char *check = (char *)malloc(need);
    sprintf(check, fmt_str, tableName);

    // Execute the command and give it the callback
    if (sqlite3_exec(db, check, callback, &exists, &sql_err) != SQLITE_OK)
      return false;

    return exists;
  }

  inline void createTable(Matrix_t matrix, unsigned short primaryKey) {
    const char *fmt_str = "CREATE TABLE IF NOT EXISTS %s (%s);";
    // example "name TEXT PRIMARY KEY,"
    // example "value REAL NOT NULL);"
    const char *names_fmt_str = "%s %s %s%s";

    size_t nameBufSize = 64;
    char *nameBuffer = (char *)malloc(nameBufSize);
    size_t pos = 0;

    for (size_t i = 0; i < matrix.colCount; ++i) {
      size_t need =
          snprintf(nameBuffer + pos, nameBufSize - pos, names_fmt_str,
                   matrix.getColumnName(i), matrix.values[i].typeString(),
                   (i == primaryKey) ? "PRIMARY KEY" : "NOT NULL",
                   (i == matrix.colCount - 1) ? ", " : "");
      if (need > nameBufSize) {
        nameBufSize *= 2;
        nameBuffer = (char *)realloc(nameBuffer, nameBufSize);
        size_t need =
            snprintf(nameBuffer + pos, nameBufSize - pos, names_fmt_str,
                     matrix.getColumnName(i), matrix.values[i].typeString(),
                     (i == primaryKey) ? "PRIMARY KEY" : "NOT NULL",
                     (i < matrix.colCount - 1) ? ", " : "");
      }
      pos += need;
    }

    size_t need = snprintf(NULL, 0, fmt_str, matrix.name, nameBuffer);
    char *buffer = (char *)malloc(need);
    sprintf(buffer, fmt_str, matrix.name, nameBuffer);

    execSimpleSQL(buffer);
  }

  inline void dropTable(const char *tableName) {
    const char *fmt_str = "DROP TABLE IF EXISTS %s;";
    size_t bufSize = snprintf(NULL, 0, fmt_str, tableName);
    char *buffer = (char *)malloc(bufSize);
    sprintf(buffer, fmt_str, tableName);
    execSimpleSQL(buffer);
  }

  inline void insertInto(Matrix_t matrix, Row_t data) {
    if (data.colCount != matrix.colCount)
      return;

    const char *fmt_str = "INSERT INTO %s (%s) VALUES (%s);";
    size_t bufSize = snprintf(
        NULL, 0, fmt_str, matrix.getSQLColumnNamesString(), data.toSQLString());

    char *sql_str = (char *)malloc(bufSize);
    sprintf(sql_str, fmt_str, matrix.getSQLColumnNamesString(),
            data.toSQLString());

    execSimpleSQL(sql_str);
  }

  inline void insertManySameTypeInto(Matrix_t matrix, Row_t *data,
                                     unsigned long rowCount) {

    if (data->colCount != matrix.colCount)
      return;

    short colCount = matrix.colCount;
    sqlite3_stmt *stmt;

    execSimpleSQL(strdup("BEGIN TRANSACTION;"));

    const char *fmt_str = "INSERT INTO %s (%s) VALUES (%s);";

    for (unsigned long i = 0; i < rowCount; ++i) {
      size_t bufSize =
          snprintf(NULL, 0, fmt_str, matrix.getSQLColumnNamesString(),
                   data.toSQLString());

      char *sql_str = (char *)malloc(bufSize);
      sprintf(sql_str, fmt_str, matrix.getSQLColumnNamesString(),
              data.toSQLString());

      if (sqlite3_prepare_v2(db, sql_str, -1, &stmt, nullptr) != SQLITE_OK)
        throw std::runtime_error(db_error_msg("Prepare"));

      if (sqlite3_step(stmt) != SQLITE_DONE)
        throw std::runtime_error(db_error_msg("Step"));

      sqlite3_reset(stmt);

      sqlite3_finalize(stmt);
    }

    execSimpleSQL(strdup("COMMIT;"));
  }

  inline Matrix_t selectFromTable(const char *tableName) {
    Matrix_t matrix;

    size_t bufSize = snprintf(NULL, 0, "SELECT * FROM %s;", tableName);
    char *sql_str = (char *)malloc(bufSize);
    sprintf(sql_str, "SELECT * FROM %s", tableName);

    return queryToTable(sql_str);
  }

private:
  sqlite3 *db;
  std::string filename;
  char *sql_err;

  inline Matrix_t queryToTable(const char *query) {
    Matrix_t selection;
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
      throw std::runtime_error(db_error_msg("Prepare"));
    }

    size_t colCount = sqlite3_column_count(stmt);
    selection = Matrix_t(colCount);

    for (size_t i = 0; i < colCount; ++i) {
      strcpy(selection.columnNames + (i * MAX_COLUMN_NAME_LENGTH),
             sqlite3_column_name(stmt, i));
      printf("%s", selection.columnNames + (i * MAX_COLUMN_NAME_LENGTH));
    }

    Row_t r = Row_t(colCount);
    do {
      for (size_t i = 0; i < colCount; ++i) {
        r.values[i].from_column(stmt, i);
      }
      selection.appendRow(r);
    } while (sqlite3_step(stmt) == SQLITE_ROW);

    sqlite3_finalize(stmt);
    return selection;
  }

  inline void execSimpleSQL(const char *sql_str) {
    if (sqlite3_exec(db, sql_str, nullptr, nullptr, &sql_err) != SQLITE_OK)
      throw std::runtime_error(sql_error());
  }

  inline const char *db_error_msg(const char *error) {
    size_t bufSize =
        snprintf(NULL, 0, "%s Error: %s", error, sqlite3_errmsg(db));
    char *buffer = (char *)malloc(bufSize);
    sprintf(buffer, "%s Error: %s", error, sqlite3_errmsg(db));
    return buffer;
  }

  inline const char *sql_error() {
    size_t bufSize = snprintf(NULL, 0, "SQL Error: %s", sql_err);
    char *buffer = (char *)malloc(bufSize);
    sprintf(buffer, "SQL Error: %s", sql_err);
    return buffer;
  }
};

} // namespace SQL
#endif
