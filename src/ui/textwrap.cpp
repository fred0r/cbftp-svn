#include "textwrap.h"

std::list<std::string> TextWrap::wrap(const std::string & text, unsigned int linelen) {
  std::list<std::string> out;
  if (linelen == 0) {
    out.push_back(text);
    return out;
  }
  size_t linestart = 0;
  size_t lastspace = 0;
  for (size_t i = 0; i < text.length(); i++) {
    bool atspace = text[i] == ' ';
    if (!atspace && i - linestart + 1 <= linelen) {
      continue;
    }
    if (i - linestart + 1 > linelen) {
      size_t breakpos = (lastspace > linestart) ? lastspace : i;
      out.emplace_back(text.substr(linestart, breakpos - linestart));
      linestart = breakpos;
      while (linestart < text.length() && text[linestart] == ' ') {
        linestart++;
      }
      lastspace = linestart;
      i = linestart - 1;
      continue;
    }
    if (atspace && i > linestart && text[i - 1] == '.') {
      out.emplace_back(text.substr(linestart, i - linestart));
      lastspace = i + 1;
      linestart = lastspace;
    }
    else {
      lastspace = i;
    }
  }
  if (linestart < text.length()) {
    out.emplace_back(text.substr(linestart));
  }
  return out;
}
