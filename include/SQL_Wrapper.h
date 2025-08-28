#ifndef SQL_DB_H
#define SQL_DB_H

#include "SQL_Datatypes.h"
#include <cstdlib>
#include <cstring>
#include <ostream>

#ifndef ARDUINO
#include <format>
#include <iostream>
#include <stdexcept>
#else
#include <Arduino>
#endif

namespace SQL {

class SQL_DB {

public:
  SQL_DB(const char *filename) : filename(filename) {
    // Check if we can open the DB, do so
    if (sqlite3_open_v2(filename, &db,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                        nullptr) != SQLITE_OK)
      throw std::runtime_error(std::format(
          "Open failed:\n, {} : {}", sqlite3_errcode(db), sqlite3_errmsg(db)));

    sql_err = nullptr;
  }

  ~SQL_DB() {
    sqlite3_close_v2(db);
    if (sql_err)
      sqlite3_free(sql_err);
  }

  inline bool tableExists(const char *tableName) {
    // This bool and call back are used to check if the table exists in the db
    bool exists = false;
    auto callback = [](void *data, int argc, char **argv,
                       char **colNames) -> int {
      bool *exists_ptr = static_cast<bool *>(data);
      *exists_ptr = (argc > 0 && argv[0] != nullptr);
      return 0;
    };

    std::string check = std::format("SELECT name "
                                    "FROM sqlite_master "
                                    "WHERE type='table' AND name='{}';",
                                    tableName);

    // Execute the command and give it the callback
    if (sqlite3_exec(db, check.c_str(), callback, &exists, &sql_err) !=
        SQLITE_OK)
      throw std::runtime_error(sql_error());

    return exists;
  }

  inline void createTable(Matrix_t matrix, unsigned short primaryKey) {
    std::string sql_str =
        std::format("CREATE TABLE IF NOT EXISTS {}(", matrix.name);

    for (short i = 0; i < matrix.colCount; ++i) {
      sql_str += std::format("{} {} {}{}", matrix.columnNames[i],
                             matrix.values[i][0].typeString(),
                             (i == primaryKey) ? "PRIMARY KEY" : "NOT NULL",
                             (i != (matrix.colCount - 1)) ? "," : ");");
    }
    execSimpleSQL(strdup(sql_str.c_str()));
  }

  inline void dropTable(const char *tableName) {
    execSimpleSQL(
        strdup(std::format("DROP TABLE IF EXISTS {};", tableName).c_str()));
  }

  inline void insertInto(Matrix_t matrix, Row_t data) {
    if (data.colCount != matrix.colCount)
      return;

    std::string sql_str = std::format("INSERT INTO {}", matrix.name);
    std::string dName_str = "(";
    std::string data_str = "VALUES (";
    for (short i = 0; i < matrix.colCount; ++i) {
      bool last = matrix.colCount - i > 1;
      insert(dName_str, matrix.getColumnName(i), last);
      insert(data_str, data.values[i].toString(), last);
    }

    sql_str = std::format("{} {} {};", sql_str, dName_str, data_str);

    execSimpleSQL(strdup(sql_str.c_str()));
  }

  inline void insertManySameTypeInto(Matrix_t matrix, Row_t *data,
                                     unsigned long rowCount) {

    if (data->colCount != matrix.colCount)
      return;

    short colCount = matrix.colCount;
    sqlite3_stmt *stmt;

    execSimpleSQL(strdup("BEGIN TRANSACTION;"));

    for (unsigned long i = 0; i < rowCount; ++i) {
      std::string sql_str = std::format("INSERT INTO {}", matrix.name);
      std::string dName_str = "(";
      std::string data_str = "VALUES (";
      for (long i = 0; i < colCount; ++i) {
        bool last = i != (colCount - 1);
        insert(dName_str, matrix.columnNames[i], last);
        insert(data_str, data->values[i].toString(), last);
      }

      sql_str = std::format("{} {} {};", sql_str, dName_str, data_str);

      if (sqlite3_prepare_v2(db, sql_str.c_str(), -1, &stmt, nullptr) !=
          SQLITE_OK)
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

    std::string sql_str = std::format("SELECT * FROM {};", tableName);
    return queryToTable(strdup(sql_str.c_str()));
  }

private:
  sqlite3 *db;
  std::string filename;
  char *sql_err;

  inline void insert(std::string &dest, const char *str, bool last) {
    dest += std::format("{}{}", str, (last) ? ", " : ")");
  }

  inline Matrix_t queryToTable(char *query) {
    Matrix_t selection;
    sqlite3_stmt *stmt;

    printf("1: %p %s \n", (void *)query, query);

    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
      throw std::runtime_error(db_error_msg("Prepare"));
    }

    printf("2");

    unsigned short colCount = sqlite3_column_count(stmt);
    selection = Matrix_t(colCount);

    printf("3");

    for (int i = 0; i < colCount; ++i) {
      snprintf(selection.columnNames[i], MAX_COLUMN_NAME_LENGTH, "%s",
               sqlite3_column_name(stmt, i));
      printf("%s", selection.columnNames[i]);
    }

    printf("4");

    Row_t r = Row_t(colCount);
    do {
      for (int i = 0; i < colCount; ++i) {
        std::cout << "1";
        r.values[i].from_column(stmt, i);
        std::cout << "2";
      }
      selection.appendRow(r);
      std::cout << "3";
    } while (sqlite3_step(stmt) == SQLITE_ROW);

    sqlite3_finalize(stmt);
    free(query);
    return selection;
  }

  inline void execSimpleSQL(char *sql_str) {
    if (sqlite3_exec(db, sql_str, nullptr, nullptr, &sql_err) != SQLITE_OK)
      throw std::runtime_error(sql_error());
    free(sql_str);
  }

  inline std::string db_error_msg(const char *error) {
    return std::format("{} Error: {}", error, sqlite3_errmsg(db));
  }

  inline std::string sql_error() {
    std::string str = std::format("SQL Error: {}", sql_err);
    return str;
  }
};

} // namespace SQL
#endif
