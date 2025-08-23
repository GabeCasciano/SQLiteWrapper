#ifndef SQL_DB_H
#define SQL_DB_H

#include "SQL_Datatypes.h"
#include "SQL_Value.h"

#ifndef ARDUINO
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <format>
#include <iostream>
#include <stdexcept>
#include <utility>
#else
#include <Arduino>
#endif

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

  ~SQL_DB() { sqlite3_close_v2(db); }

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

  inline void createTable(const char *tableName, Row_t rowOfNames) {
    std::string sql_str =
        std::format("CREATE TABLE IF NOT EXISTS {}(", tableName);

    for (short i = 0; i < rowOfNames.colCount; ++i) {
      sql_str += std::format("{} {} {}{}", rowOfNames.columns[i].name,
                             rowOfNames.columns[i].value.typeString(),
                             (rowOfNames.columns[i].primaryKey) ? "PRIMARY KEY"
                                                                : "NOT NULL",
                             (i != (rowOfNames.colCount - 1)) ? "," : ");");
    }
    execSimpleSQL(sql_str.c_str());
  }

  inline void dropTable(const char *tableName) {
    execSimpleSQL(std::format("DROP TABLE IF EXISTS {};", tableName).c_str());
  }

  inline void insertInto(const char *tableName, Row_t data) {
    std::string sql_str = std::format("INSERT INTO {}", tableName);
    std::string dName_str = "(";
    std::string data_str = "VALUES (";
    for (short i = 0; i < data.colCount; ++i) {
      char *str;
      bool last = i != (data.colCount - 1);

      insert(dName_str, data.columns[i].name, last);
      insert(data_str, data.columns[i].value.toString(str), last);

      free(str);
    }

    sql_str = std::format("{} {} {};", sql_str, dName_str, data_str);
    execSimpleSQL(sql_str.c_str());
  }

  inline void insertManySameTypeInto(const char *tableName, Row_t *data,
                                     long rowCount) {

    execSimpleSQL("BEGIN TRANSACTION;");

    short colCount = data[0].colCount;

    sqlite3_stmt *stmt;

    std::string sql_str = std::format("INSERT INTO {}", tableName);
    std::string dName_str = "(";
    std::string data_str = "VALUES (";
    for (long i = 0; i < colCount; ++i) {
      char *str;
      bool last = i != (colCount - 1);
      insert(dName_str, data->columns[i].name, last);
      insert(data_str, data->columns[i].value.toString(str), last);
    }

    sql_str = std::format("{} {} {};", sql_str, dName_str, data_str);

    if (sqlite3_prepare_v2(db, sql_str.c_str(), -1, &stmt, nullptr) !=
        SQLITE_OK)
      throw std::runtime_error(db_error_msg("Prepare"));

    if (sqlite3_step(stmt) != SQLITE_DONE)
      throw std::runtime_error(db_error_msg("Step"));

    sqlite3_reset(stmt);

    sqlite3_finalize(stmt);

    execSimpleSQL("COMMIT;");
  }

  Table selectFromTable(const char *tableName, Row_t colNames) {
    Table table(colNames);

    std::string sql_str = "SELECT ";
    for (short i = 0; i < colNames.colCount; ++i)
      insert(sql_str, colNames.columns[i].name, (i != (colNames.colCount - 1)));

    sql_str = std::format("{} FROM {};", sql_str, tableName);
    return queryToTable(sql_str.c_str());
  }

  inline Table selectAllFromTable(const char *tableName) {
    Table table;

    std::string sql_str = std::format("SELECT * FROM {};", tableName);
    return queryToTable(sql_str.c_str());
  }

private:
  sqlite3 *db;
  std::string filename;
  char *sql_err;

  inline void insert(std::string &dest, const char *str, bool last) {
    dest += std::format("{}{}", str, (!last) ? ", " : ")");
  }

  inline Table queryToTable(const char *query) {
    Table selection;
    SqlValue *rowOfData;
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK)
      throw std::runtime_error(db_error_msg("Prepare"));

    selection = Table(sqlite3_column_count(stmt));
    rowOfData = new SqlValue[selection.colCount];

    for (int i = 0; i < selection.colCount; ++i)
      selection.setColName(sqlite3_column_name(stmt, i), i);

    do {
      for (int i = 0; i < selection.colCount; ++i)
        rowOfData[i].from_column(stmt, i);
      selection.appendRow(rowOfData);
    } while (sqlite3_step(stmt) == SQLITE_ROW);

    sqlite3_finalize(stmt);
    free(rowOfData);
    return selection;
  }

  inline void execSimpleSQL(const char *sql_str) {
    if (sqlite3_exec(db, sql_str, nullptr, nullptr, &sql_err) != SQLITE_OK)
      throw std::runtime_error(sql_error());
  }

  inline std::string db_error_msg(const char *error) {
    return std::format("{} Error: {}", error, sqlite3_errmsg(db));
  }

  inline std::string sql_error() {
    std::string str = std::format("SQL Error: {}", sql_err);
    sqlite3_free(sql_err);
    return str;
  }
};
#endif
