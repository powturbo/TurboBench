
#include <sys/types.h>
#include <sys/stat.h>
#include <string>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "utilities.hpp"

#ifdef WINDOWS
  #include <windows.h>
#else
  #include <sys/stat.h>
  #include <sys/time.h>
  #include <sys/types.h>
#endif

using namespace std;

#ifndef WINDOWS
unsigned int GetTickCount()
{ 
       struct timeval tv;
       gettimeofday( &tv, NULL );
       return (unsigned int)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
 }
#endif

char *absolute_path(char *source, char *destination)
{
#ifdef WINDOWS
	return _fullpath(destination, source, 10000);
#else
	return realpath(source, destination);
#endif
}

string ucase(string str)
{
	//change each element of the string to upper case
   for(unsigned int i=0;i<str.length();i++)
   {
      str[i] = toupper(str[i]);
   }
   return str;
}

string lcase(string str)
{
	//change each element of the string to lower case
	string s = str;
	for(unsigned int i=0;i<s.length();i++)
   {
      s[i] = tolower(s[i]);
   }
   return s;
}

void utils_yield(void)
{
// don't use sleep(0) because it yields multiple time slices
#ifdef WINDOWS
    sched_yield();
#elif defined(__SVR4) && defined(__sun)
    yield(); // Solaris
#else
    sched_yield();  // other *nix
#endif
}

/* reverse:  reverse string s in place */
void reverse(char s[])
{
    size_t j;
    unsigned char c;
    unsigned int i;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/* itoa:  convert n to characters in s */
void itoa(int n, char s[])
{
    int i, sign;

    if ((sign = n) < 0)  /* record sign */
        n = -n;          /* make n positive */
    i = 0;
    do {       /* generate digits in reverse order */
        s[i++] = n % 10 + '0';   /* get next digit */
    } while ((n /= 10) > 0);     /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
} 


void *aligned_malloc(size_t size, size_t alignment)
{
    void *pa, *ptr;
    pa = malloc((size + alignment - 1) + 2*sizeof(void *));
    if(!pa)
        return NULL;
    ptr = (void *)(((size_t)pa + 2*sizeof(void *) + alignment - 1) & ~(alignment - 1));
    *((void **)ptr - 1) = pa;
    *((size_t *)ptr - 2) = size;
    return ptr;
}

void aligned_free(void *ptr)
{
    free(*((void **)ptr - 1));
}

void *aligned_realloc(void *ptr, size_t size, size_t alignment)
{
    void *p = aligned_malloc(size, alignment);
    if(!p)
        return NULL;
    if(!ptr)
        return p;
    size_t c = *((size_t *)ptr - 2);
    if(c > size)
        c = size;
    memmove(p, *((void **)ptr - 1), c);
    aligned_free(ptr);
    return p;
}

unsigned int adler(unsigned char *data, size_t len, unsigned int crc) 
{
    unsigned int a = crc >> 16;
    unsigned int b = crc & 0xffff;
    while (len > 0)
    {
        size_t tlen = len > 5550 ? 5550 : len;
        len -= tlen;
        do 
        {
            a += *data++;
            b += a;
        } while (--tlen);
 
        a %= 65521;
        b %= 65521;
    } 
    return (b << 16) | a;
}


bool exists(string file)
{
// warning: may return false even though file does exist, but not vice versa
    FILE *f = fopen(file.c_str(), "r");
    if (f == 0)
        return false;
    else
    {
        fclose(f);
        return true;
    }
}

string remove_leading_curdir(string path)
{
	if(path.length() >= 2 && (path.substr(0, 2) == ".\\" || path.substr(0, 2) == "./"))
		return path.substr(2, path.length() - 2);
	else
		return path;
}

string remove_delimitor(string path)
{
    size_t r = path.find_last_of("/\\");
	if(r == path.length() - 1)
		return path.substr(0, r);
	else
		return path;
}


bool is_dir(string path) 
{
    struct stat st_ifile;
#ifdef WINDOWS
	if(path.length() == 2 && path.substr(1, 1) == ":")
		path = path + "\\";
	else if(path.length() == 3 && (path.substr(1, 2) == ":\\" || path.substr(1, 1) == ":/"))
	{
		// drive letter, do nothing
	}
	else
		path = remove_delimitor(path);
#endif

	stat(path.c_str(), &st_ifile);

#ifdef WINDOWS
	const DWORD attributes = ::GetFileAttributes(path.c_str());

	if (INVALID_FILE_ATTRIBUTES == attributes)
	{
       const DWORD error = ::GetLastError();

       if (ERROR_FILE_NOT_FOUND == error || ERROR_PATH_NOT_FOUND == error)
			return false;
       else
			return false;
	}
	return ((FILE_ATTRIBUTE_DIRECTORY & attributes) != 0);
#else
    return S_ISDIR(st_ifile.st_mode);
#endif

}

string str(int intValue) 
{ 
    char myBuff[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; 
    itoa(intValue,myBuff); 
    return((string)myBuff); 
}

string delimiter(long long l)
{
    char s[25], d[25];
    unsigned int i, j = 0;

    memset(s, 0, 25);
    memset(d, 0, 25);    
#ifdef WINDOWS
    sprintf(s, "%I64d", l);
#else
    sprintf(s, "%lld", l);
#endif
    for(i = 0; i < strlen(s); i++)
    {
        if((strlen(s) - i) % 3 == 0 && i != 0)
        {
            d[j] = ',';
            j++;
        }
        d[j] = s[i];
        j++;
    }    
    return string(d);
}

