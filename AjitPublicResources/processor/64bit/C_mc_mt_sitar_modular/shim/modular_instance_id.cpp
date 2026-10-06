#include "modular_instance_id.h"

#include <cctype>
#include <climits>

namespace {

bool parse_bracket_index(const char** cursor, int* value)
{
  if (cursor == nullptr || *cursor == nullptr || value == nullptr) {
    return false;
  }

  const char* p = *cursor;
  while (*p != '\0' && *p != '[') {
    ++p;
  }
  if (*p != '[') {
    return false;
  }
  ++p;
  if (!std::isdigit((unsigned char) *p)) {
    return false;
  }

  long parsed = 0;
  while (std::isdigit((unsigned char) *p)) {
    const int digit = *p - '0';
    if (parsed > (LONG_MAX - digit) / 10) {
      return false;
    }
    parsed = (parsed * 10) + digit;
    ++p;
  }
  if (*p != ']' || parsed > INT_MAX) {
    return false;
  }

  *value = (int) parsed;
  *cursor = p + 1;
  return true;
}

} // namespace

extern "C" int modular_extract_first_index(const char* src, int* first)
{
  const char* cursor = src;
  return parse_bracket_index(&cursor, first) ? 1 : 0;
}

extern "C" int modular_extract_two_indices(const char* src, int* first, int* second)
{
  const char* cursor = src;
  return (parse_bracket_index(&cursor, first) &&
          parse_bracket_index(&cursor, second)) ? 1 : 0;
}
