#include "util.h"

#include <csignal>
#include <cstring>
#include <sstream>

namespace Core {
namespace util {


int wildcmp(const char* wild, const char* string) {
  const char* cp = nullptr;
  const char* mp = nullptr;
  while ((*string) && (*wild != '*')) {
    if (*wild != *string && *wild != '?' &&
        !(*wild >= 65 && *wild <= 90 && *wild + 32 == *string) &&
        !(*wild >= 97 && *wild <= 122 && *wild - 32 == *string)) {
      return 0;
    }
    wild++;
    string++;
  }
  while (*string) {
    if (*wild == '*') {
      if (!*++wild) {
        return 1;
      }
      mp = wild;
      cp = string+1;
    } else if (*wild == *string || *wild == '?' ||
    (*wild >= 65 && *wild <= 90 && *wild + 32 == *string) ||
    (*wild >= 97 && *wild <= 122 && *wild - 32 == *string)) {
      wild++;
      string++;
    } else {
      wild = mp;
      string = cp++;
    }
  }
  while (*wild == '*') {
    wild++;
  }
  return !*wild;
}

int wildcmpCase(const char *wild, const char *string) {
  const char* cp = nullptr;
  const char* mp = nullptr;
  while ((*string) && (*wild != '*')) {
    if ((*wild != *string) && (*wild != '?')) {
      return 0;
    }
    wild++;
    string++;
  }
  while (*string) {
    if (*wild == '*') {
      if (!*++wild) {
        return 1;
      }
      mp = wild;
      cp = string+1;
    } else if ((*wild == *string) || (*wild == '?')) {
      wild++;
      string++;
    } else {
      wild = mp;
      string = cp++;
    }
  }
  while (*wild == '*') {
    wild++;
  }
  return !*wild;
}

std::string getStrError(int error) {
  char buf[ERR_BUF_SIZE];
#if defined(_GNU_SOURCE)
  char* ret = strerror_r(error, buf, sizeof(buf));
  if (ret) {
    return ret;
  }
  return buf;
#else
  int ret = strerror_r(error, buf, sizeof(buf));
  if (ret == 0) {
    return buf;
  }
  return "Unknown error " + std::to_string(error);
#endif
}

} // namespace util
} // namespace Core
