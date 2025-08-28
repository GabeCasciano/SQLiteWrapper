#ifndef SQL_ROW
#define SQL_ROW

#include "SQL_Value.h"

namespace SQL {
struct Row_t {
  unsigned short colCount = 0;
  SqlValue *values = nullptr;
  Row_t() = default;
  Row_t(unsigned short colCount) : colCount(colCount) {
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

  void insertValue(SqlValue value, unsigned short cIdx) {
    if (cIdx >= colCount)
      return;
    if (values != nullptr)
      values[cIdx] = value;
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

    for (unsigned short i = 0; i < colCount; ++i)
      values[i] = o.values[i];
  }

  void move_from(Row_t &&o) noexcept {
    destroy();
    this->colCount = o.colCount;
    this->values = new SqlValue[colCount];
    for (unsigned short i = 0; i < colCount; ++i)
      values[i] = std::move(o.values[i]);
    o.destroy();
  }
};
} // namespace SQL

#endif // !SQL_ROW
