#ifndef SQL_UTIL_H_
#define SQL_UTIL_H_

#ifndef ARDUINO
#include <cstring>
#else
#include <Arduino>
#endif

namespace SQL {

inline void safeNameCopy(char *dest, const char *src, size_t len) {
  if (strlen(src) < len)
    strcpy(dest, src);
  else
    memcpy(dest, src, len);
}
} // namespace SQL
#endif
