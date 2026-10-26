// Host test for the session-cookie parser. Build and run:
//   g++ -std=c++17 -Wall -Itests/host tests/host/test_cookie.cpp -o /tmp/test_cookie && /tmp/test_cookie
//
// Regression: renaming the cookie from "qs_sess" (7 characters + '=') to "seq_sess" (8 + '=') while the parser
// still skipped a fixed 8 characters made the token 33 characters long, so every login was rejected.

#include <cstdio>
#include "Arduino.h"
#include "../../firmware/ProgrammableSequencer/Cookie.h"

static int fails = 0;
#define CHECK(cond)                                                       \
  do {                                                                    \
    if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); fails++; } \
  } while (0)

int main() {
  const String tok("0123456789abcdef0123456789abcdef");   // 32 hex characters, as newSession() makes

  // The exact header the browser sends after login.
  CHECK(cookieValue(String("seq_sess=") + tok, "seq_sess") == tok);
  CHECK(cookieValue(String("seq_sess=") + tok, "seq_sess").length() == 32);   // the bug made this 33

  // Other cookies around it.
  CHECK(cookieValue(String("a=1; seq_sess=") + tok + "; b=2", "seq_sess") == tok);
  CHECK(cookieValue(String("a=1; seq_sess=") + tok, "seq_sess") == tok);
  CHECK(cookieValue(String("seq_sess=") + tok + "; a=1", "seq_sess") == tok);

  // A longer or similar name must not be mistaken for ours.
  CHECK(cookieValue(String("xseq_sess=BAD; seq_sess=") + tok, "seq_sess") == tok);
  CHECK(cookieValue(String("xseq_sess=BAD"), "seq_sess") == "");

  // Absent, empty, and cleared (logout sends an empty value).
  CHECK(cookieValue("", "seq_sess") == "");
  CHECK(cookieValue("a=1; b=2", "seq_sess") == "");
  CHECK(cookieValue("seq_sess=", "seq_sess") == "");

  // Works for any cookie name length.
  CHECK(cookieValue("qs_sess=abc", "qs_sess") == "abc");
  CHECK(cookieValue("very_long_cookie_name=abc", "very_long_cookie_name") == "abc");

  if (fails) { std::printf("%d check(s) failed\n", fails); return 1; }
  std::printf("cookie parser: all checks passed\n");
  return 0;
}
