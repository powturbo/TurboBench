#ifndef _CSA_COMMON_H_
#define _CSA_COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <stdexcept>
#include <fcntl.h>
#include <assert.h>
#include <csc_enc.h>
#include <csc_dec.h>

using namespace std;

#ifdef unix
#define PTHREAD 1
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <dirent.h>
#include <utime.h>
#include <errno.h>

#ifdef unixtest
struct termios {
  int c_lflag;
};
#define ECHO 1
#define ECHONL 2
#define TCSANOW 4
int tcgetattr(int, termios*) {return 0;}
int tcsetattr(int, int, termios*) {return 0;}
#else
#include <termios.h>
#endif

#else  // Assume Windows
#include <windows.h>
#include <wincrypt.h>
#include <io.h>
#endif

#ifdef _MSC_VER  // Microsoft C++
#define fseeko(a,b,c) _fseeki64(a,b,c)
#define ftello(a) _ftelli64(a)
#else
#ifndef unix
#ifndef fseeko
#define fseeko(a,b,c) fseeko64(a,b,c)
#endif
#ifndef ftello
#define ftello(a) ftello64(a)
#endif
#endif
#endif

// Convert seconds since 0000 1/1/1970 to 64 bit decimal YYYYMMDDHHMMSS
// Valid from 1970 to 2099.
int64_t decimal_time(time_t tt);

// Convert decimal date to time_t - inverse of decimal_time()
time_t unix_time(int64_t date);

inline int tolowerW(int c) {
#ifndef unix
  if (c>='A' && c<='Z') return c-'A'+'a';
#endif
  return c;
}


static const string DUMMY_FILENAME = "****";

// In Windows, convert 16-bit wide string to UTF-8 and \ to /
#ifndef unix
string wtou(const wchar_t* s);

// In Windows, convert UTF-8 string to wide string ignoring
// invalid UTF-8 or >64K. If doslash then convert "/" to "\".
std::wstring utow(const char* ss, bool doslash=false);

// Print a UTF-8 string to f (stdout, stderr) so it displays properly
void printUTF8(const char* s, FILE* f);

// Print error message
void winError(const char* filename);

// Set the last-modified date of an open file handle
void setDate(HANDLE out, int64_t date);

#endif

#endif


