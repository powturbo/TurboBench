// FILE_FLAG_NO_BUFFERING (Windows only) disk I/O must be sector aligned. AIO_MAX_SECTOR_SIZE must equal 
// the largest disk sector size that can exist

#ifndef UTILITIES_HEADER
#define UTILITIES_HEADER

#include <string>

using namespace std;

#if defined(_WIN32) || defined(__WIN32__) || defined(_WIN64)
#define WINDOWS
#endif

#ifdef WINDOWS
	#include "pthread.h"
#else
	#include <pthread.h>
#endif

#define _CRT_SECURE_NO_WARNINGS
string ucase(string str);
string lcase(string str);
string remove_leading_curdir(string path);
string remove_delimitor(string path);
void utils_yield(void);
char *absolute_path(char *source, char *destination);
void abort(const char *fmt, ...);
string get_pid(void);
unsigned int adler(unsigned char *data, size_t len, unsigned int crc);
bool exists(string file);
bool is_dir(string path);
string str(int intValue);
string delimiter(long long l);
void *aligned_memory_alloc(size_t size, size_t alignment);
void aligned_free(void *ptr);
void *aligned_realloc(void *ptr, size_t size, size_t alignment);
#ifndef WINDOWS
unsigned int GetTickCount();
#endif

#endif

