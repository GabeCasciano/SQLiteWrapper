#include <cstring>
#include <exception>
#include <format>
#include <functional>
#include <iostream>

#include "SQL_Datatypes.h"
#include "SQL_Value.h"
#include "SQL_Wrapper.h"
#include <stdexcept>

using namespace SQL;

enum TestCond_t { START = 1, RUNNING = 2, SUCCESS = 3, FAIL = 4 };

void println(std::string str) { std::cout << str << std::endl; }
void logLn(TestCond_t t, std::string str) {
  switch (t) {
  case START:
    println(std::format("[START] - {}", str));
    break;
  case RUNNING:
    println(std::format("[RUNNING] - {}", str));
    break;
  case SUCCESS:
    println(std::format("[SUCCESS] - {}", str));
    break;
  case FAIL:
    println(std::format("[FAIL] - {}", str));
    break;
  }
}

void tryFunction(std::function<void()> func, std::string testName) {
  try {
    logLn(START, std::format("Starting test -> {}", testName));
    func();
    logLn(RUNNING, std::format("Ending test"));
  } catch (const std::runtime_error &r) {
    logLn(FAIL, r.what());
  } catch (const std::exception &e) {
    logLn(FAIL, e.what());
  }
  logLn(SUCCESS, testName);
}

int main() {
  println("SQL Wrapper test");

  auto create_db = []() { SQL_DB sql("test.db"); };
  tryFunction(create_db, "Create DB");

  auto open_db = []() { SQL_DB sql("test.db"); };
  tryFunction(open_db, "Openning DB");

  auto create_table = []() {
    SQL_DB sql("test.db");
    const char *colNames[2] = {"name", "value"};
    Matrix_t matrix = Matrix_t("test", 2, colNames);
    sql.createTable(matrix, 0);
  };
  tryFunction(create_table, "Open, Create");

  auto open_inset_table = []() {
    SQL_DB sql("test.db");

    const char *colNames[2] = {"name", "value"};
    const char *testName = "TestName";
    Matrix_t matrix = Matrix_t("test", 2, colNames);
    Row_t data = Row_t(2);
    data.insertValue(testName, 0);
    data.insertValue((int64_t)1, 1);

    sql.insertInto(matrix, data);
  };
  tryFunction(open_inset_table, "Open, Create, Insert");

  auto retrieve_table = []() {
    SQL_DB sql("test.db");

    println("1");
    Matrix_t matrix = sql.selectFromTable("test");
    println("2");
    println(matrix.toString());
  };
  tryFunction(retrieve_table, "Read db");

  return 0;
}
