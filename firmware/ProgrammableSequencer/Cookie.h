#pragma once
// Cookie header parsing. Kept free of any hardware or WebServer dependency so it is tested on a PC
// (tests/host/test_cookie.cpp).

#include <Arduino.h>

// Value of cookie `name` in a Cookie header such as "a=1; seq_sess=TOKEN; b=2", or "" when it is absent.
// The match must start a cookie ("xseq_sess=" does not match "seq_sess"), and the value is taken from the
// real length of the name, so renaming a cookie cannot shift the result.
static String cookieValue(const String& header, const char* name) {
  String key = String(name) + "=";
  int i = 0;
  for (;;) {
    i = header.indexOf(key, i);
    if (i < 0) return "";
    if (i == 0 || header.charAt(i - 1) == ' ' || header.charAt(i - 1) == ';') break;
    i += key.length();
  }
  int s = i + key.length();
  int e = header.indexOf(';', s);
  return header.substring(s, e < 0 ? header.length() : e);
}
