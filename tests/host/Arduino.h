#pragma once
// Minimal stand-in for Arduino's String, just enough to build the firmware headers that are tested on a PC.
// Semantics follow the Arduino core (indexOf/substring/charAt).

#include <algorithm>
#include <string>

class String {
public:
  String() {}
  String(const char* s) : s_(s ? s : "") {}
  String(const std::string& s) : s_(s) {}

  unsigned int length() const { return (unsigned int)s_.size(); }
  const char* c_str() const { return s_.c_str(); }
  char charAt(unsigned int i) const { return i < s_.size() ? s_[i] : 0; }

  int indexOf(char c, unsigned int from = 0) const {
    size_t p = s_.find(c, from);
    return p == std::string::npos ? -1 : (int)p;
  }
  int indexOf(const String& str, unsigned int from = 0) const {
    size_t p = s_.find(str.s_, from);
    return p == std::string::npos ? -1 : (int)p;
  }
  String substring(unsigned int from) const { return substring(from, length()); }
  String substring(unsigned int from, unsigned int to) const {
    if (from > to) std::swap(from, to);
    if (from >= s_.size()) return String();
    if (to > s_.size()) to = (unsigned int)s_.size();
    return String(s_.substr(from, to - from));
  }

  String operator+(const String& o) const { return String(s_ + o.s_); }
  bool operator==(const String& o) const { return s_ == o.s_; }
  bool operator!=(const String& o) const { return s_ != o.s_; }

private:
  std::string s_;
};
