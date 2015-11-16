#define _WIN32_WINNT 0x0500
#include <stdlib.h>
#include "../tornado/src/Common.h"
#include "../tornado/src/Compression.h"

// дКЪ НАПЮАНРЙХ НЬХАНЙ БН БКНФЕММШУ ОПНЖЕДСПЮУ - longjmp ЯХЦМЮКХГХПСЕР ОПНЖЕДСПЕ БЕПУМЕЦН СПНБМЪ Н ОПНХГНЬЕДЬЕИ НЬХАЙЕ
int jmpready = FALSE;
jmp_buf jumper;

bool AllocTopDown = true;

// ****************************************************************************
// MEMORY ALLOCATION **********************************************************
// ****************************************************************************

// #define _SZ_ALLOC_DEBUG
/* use _SZ_ALLOC_DEBUG to debug alloc/free operations */
#ifdef _SZ_ALLOC_DEBUG
#include <stdio.h>
int g_allocCount = 0;
int g_allocCountMid = 0;
int g_allocCountBig = 0;
#define alloc_debug_printf(x) fprintf x
#else
#define alloc_debug_printf(x)
#endif

void *MyAlloc(size_t size) throw()
{
  if (size == 0)
    return 0;
  alloc_debug_printf((stderr, "  Alloc %10d bytes; count = %10d\n", size, g_allocCount++));
  return ::malloc(size);
}

void MyFree(void *address) throw()
{
  if (address != 0)
    alloc_debug_printf((stderr, "  Free; count = %10d\n", --g_allocCount));

  ::free(address);
}

#ifdef FREEARC_WIN

void *MidAlloc(size_t size) throw()
{
  if (size == 0)
    return 0;
  alloc_debug_printf((stderr, "  Alloc_Mid %10d bytes;  count = %10d\n", size, g_allocCountMid++));
  return ::VirtualAlloc(0, size, MEM_COMMIT, PAGE_READWRITE);
}

void MidFree(void *address) throw()
{
  if (address == 0)
    return;
  if (address != 0)
  alloc_debug_printf((stderr, "  Free_Mid; count = %10d\n", --g_allocCountMid));
  ::VirtualFree(address, 0, MEM_RELEASE);
}

// Returns OS/hardware-specific size of large memory pages. Don't call it too often, better save returned value to global variable
SIZE_T GetLargePageSize()
{
  HANDLE hToken = 0;
  LUID luid;
  TOKEN_PRIVILEGES tp;
  OpenProcessToken (GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
  LookupPrivilegeValue (NULL, TEXT("SeLockMemoryPrivilege"), &luid);
  tp.PrivilegeCount = 1;
  tp.Privileges[0].Luid = luid;
  tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  AdjustTokenPrivileges (hToken, FALSE, &tp, sizeof(tp), 0, 0);
  CloseHandle (hToken);

  typedef SIZE_T (WINAPI *GetLargePageMinimumP)();
  GetLargePageMinimumP MyGetLargePageMinimum = (GetLargePageMinimumP) ::GetProcAddress(::GetModuleHandle(TEXT("kernel32.dll")), "GetLargePageMinimum");
  return MyGetLargePageMinimum? MyGetLargePageMinimum() : 0;
}

LPType DefaultLargePageMode = TRY;

void *BigAlloc (int64 size, LPType LargePageMode) throw()
{
  if (size<=0 || size>size_t(-1))  return 0;
  void *address = 0;
  if (LargePageMode==DEFAULT)
    LargePageMode = DefaultLargePageMode;

  if (LargePageMode!=DISABLE)
  {
    static SIZE_T page = GetLargePageSize();
    const DWORD MY_MEM_LARGE_PAGES = 0x20000000;
    alloc_debug_printf((stderr, "  page = %x", int(page)));
    if (page  &&  (size>=page || LargePageMode==FORCE))
      address = ::VirtualAlloc(0, roundUp(size,page), MEM_COMMIT | (AllocTopDown? MEM_TOP_DOWN : 0) | MY_MEM_LARGE_PAGES, PAGE_READWRITE);
  }

  if (address==0 && LargePageMode!=FORCE)
    address = ::VirtualAlloc(0, size, MEM_COMMIT | (AllocTopDown? MEM_TOP_DOWN : 0), PAGE_READWRITE);

  alloc_debug_printf((stderr, "  addr = %p;  count = %5d;  Alloc_Big %10d bytes\n", address, g_allocCountBig++, size));
  return address;
}

void BigFree(void *address) throw()
{
  if (address == 0)
    return;
  alloc_debug_printf((stderr, "  addr = %p;  count = %5d;  Free_Big\n", address, --g_allocCountBig));

  ::VirtualFree(address, 0, MEM_RELEASE);
}

#endif


// ****************************************************************************
// тСМЙЖХХ ОЮПЯХМЦЮ Х ЮПХТЛЕРХЙХ **********************************************
// ****************************************************************************

// йНОХПСЕР ЯРПНВЙС from Б to, МН МЕ АНКЕЕ len ЯХЛБНКНБ
void strncopy( char *to, char *from, int len ) {
  for (int i = len; --i && *from; )     *to++ = *from++;
  *to = '\0';
}

// пЮГАХРЭ ЯРПНЙС str МЮ ОНДЯРПНЙХ, ПЮГДЕК╦ММШЕ ЯХЛБНКНЛ splitter.
// пЕГСКЭРЮР - Б ЯРПНЙЕ str splitter ГЮЛЕМЪЕРЯЪ МЮ '\0'
//   Х ЛЮЯЯХБ result ГЮОНКМЪЕРЯЪ ЯЯШКЙЮЛХ МЮ БШДЕКЕММШЕ Б str ОНДЯРПНЙХ + NULL (ЮМЮКНЦХВМН argv)
// бНГБПЮЫЮЕР ВХЯКН МЮИДЕММШУ ОНДЯРПНЙ
int split (char *str, char splitter, char **result_base, int result_size)
{
  char **result      = result_base;
  char **result_last = result_base+result_size-1;
  *result++ = str;
  while (*str && result < result_last)
  {
    while (*str && *str!=splitter) str++;
    if (*str) {
      *str++ = '\0';
      *result++ = str;
    }
  }
  *result = NULL;
  return result-result_base;
}

// нАЗЕДХМХРЭ NULL-terminated ЛЮЯЯХБ ЯРПНЙ src Б ЯРПНЙС result, ЯРЮБЪ ЛЕФДС ЯРПНЙЮЛХ ПЮГДЕКХРЕКЭ splitter
void join (char **src, char splitter, char *result, int result_size)
{
  char *dst = result;  *dst = '\0';
  while (*src && result+result_size-dst-1 > 0)
  {
    if (dst > result)  *dst++ = splitter;
    strncopy(dst, *src++, result+result_size-dst);
    dst = strchr(dst,'\0');
  }
}

// мЮИРХ ОЮПЮЛЕРП Я ГЮДЮММШЛ ХЛЕМЕЛ Б ЛЮЯЯХБЕ ОЮПЮЛЕРПНБ ЮКЦНПХРЛЮ
char *search_param(char **param, char *prefix)
{
  for ( ; *param; param++)
    if (start_with(*param, prefix))
      return *param+strlen(prefix);
  return NULL;
}

// гЮЛЕМЪЕР Б ЯРПНЙЕ original БЯЕ БУНФДЕМХЪ from МЮ to,
// БНГБПЮЫЮЪ БМНБЭ БШДЕКЕММСЧ new ЯРПНЙС Х НЯБНАНФДЮЪ НПХЦХМЮК, ЕЯКХ АШКЮ УНРЭ НДМЮ ГЮЛЕМЮ
char *subst (char *original, char *from, char *to)
{
  while(1) {
    char *p = strstr (original, from);
    if (!p)  return original;
    char *newstr = new char[strlen(original)+strlen(to)-strlen(from)+1];
    memcpy (newstr, original, p-original);
    strcpy (newstr+(p-original), to);
    strcat (newstr+(p-original), p+strlen(from));
    delete (original);
    original = newstr;
  }
}

// оПНОСЯЙЮЕР ОПНАЕКШ Б МЮВЮКЕ ЯРПНЙХ Х САХПЮЕР ХУ Б ЙНМЖЕ, ЛНДХТХЖХПСЪ ЯРПНЙС
char *trim_spaces(char *s)
{
  while(isspace(*s)) s++;
  char *last = &last_char(s);
  while(last>=s && isspace(*last))  *last-- = '\0';
  return s;
}

// Replace from:how_many substring and put result in new allocated area
char *str_replace_n (char *orig, char *from, int how_many, char *to)
{
  char *result = new char [strlen(orig) + strlen(to) - how_many + 1], *p=result;
  memcpy(p, orig, from-orig); p += from-orig;
  strcpy(p, to);
  strcat(p, from+how_many);
  return result;
}

// Replace substring and put result in new allocated area
char *str_replace (char *orig, char *from, char *to)
{
  char *p = strstr(orig, from);
  if (p)  return str_replace_n (orig, p, strlen(from), to);
  else    return strdup_msg (orig);
}

// If the string param contains an integer, return it - otherwise set error=1
MemSize parseInt (char *param, int *error)
{
  MemSize n=0;
  char c = *param=='='? *++param : *param;
  if (c=='\0') *error=1;
  while (c>='0' && c<='9')  n=n*10+c-'0', c=*++param;
  if (c!='\0') *error=1;
  return n;
}

// Similar to parseInt, but the string param may have a suffix b/k/m/g/^, representing units of memory, or in the case of '^' (default, overridden by spec parameter), the relevant power of 2
uint64 parseMem64 (char *param, int *error, char spec)
{
  uint64 n=0;
  char c = *param=='='? *++param : *param;
  if (! (c>='0' && c<='9'))  {*error=1; return 0;}
  while (c>='0' && c<='9')   n=n*10+c-'0', c=*++param;
  switch (c? c : spec)
  {
    case 'b':  return n;
    case 'k':  return n*kb;
    case 'm':  return n*mb;
    case 'g':  return n*gb;
    case '^':  return MemSize(1)<<n;
  }
  *error=1; return 0;
}

MemSize parseMem (char *param, int *error, char spec)
{
  return parseMem64 (param, error, spec);
}

// Returns a string with the amount of memory
void showMem (MemSize mem, char *result)
{
       if (mem%gb==0) sprintf (result, "%.0lfgb", double(mem/gb));
  else if (mem%mb==0) sprintf (result, "%.0lfmb", double(mem/mb));
  else if (mem%kb==0) sprintf (result, "%.0lfkb", double(mem/kb));
  else                sprintf (result, "%.0lfb",  double(mem));
}

// Returns a string with the amount of memory
void showMem64 (uint64 mem, char *result)
{
  if(mem%terabyte==0) sprintf (result, "%.0lftb", double(mem/terabyte));
  else if (mem%gb==0) sprintf (result, "%.0lfgb", double(mem/gb));
  else if (mem%mb==0) sprintf (result, "%.0lfmb", double(mem/mb));
  else if (mem%kb==0) sprintf (result, "%.0lfkb", double(mem/kb));
  else                sprintf (result, "%.0lfb",  double(mem));
}


// йНДХПНБЮМХЕ ЯРПНЙХ Б ЬЕЯРМЮДЖЮРЕПХВМШИ БХД ОКЧЯ \0
void encode16 (const BYTE *src, int srcSize, char *dst)
{
    for( ; srcSize--; src++)
        *dst++ = int2char(*src/16),
        *dst++ = int2char(*src%16);
    *dst++ = '\0';
}

// дЕЙНДХПНБЮМХЕ ЯРПНЙХ, ГЮОХЯЮММНИ Б ЬЕЯРМЮДЖЮРЕПХВМНЛ БХДЕ, Б ОНЯКЕДНБЮРЕКЭМНЯРЭ АЮИР
void decode16 (const char *src, BYTE *dst)
{
    for( ; src[0] && src[1]; src+=2)
        *dst++ = char2int(src[0]) * 16 + char2int(src[1]);
}

// ньханвмне ДЕЙНДХПНБЮМХЕ ЯРПНЙХ, ГЮОХЯЮММНИ Б ЬЕЯРМЮДЖЮРЕПХВМНЛ БХДЕ, Б ОНЯКЕДНБЮРЕКЭМНЯРЭ АЮИР
void buggy_decode16 (const char *src, BYTE *dst)
{
    for( ; src[0] && src[1]; src+=2)
        *dst++ = buggy_char2int(src[0]) * 16 + buggy_char2int(src[1]);
}

// нЙПСЦКЪЕР ПЮГЛЕП ОЮЛЪРХ БМХГ ДН СДНАМНИ БЕКХВХМШ
MemSize rounddown_mem (MemSize n)
{
         if (n < 32*kb) return n;
    else if (n < 32*mb) return roundDown (n, 1*kb);
    else                return roundDown (n, 1*mb);
}


// ****************************************************************************
// Filename manipulations *****************************************************
// ****************************************************************************

// Replace alternative path delimiters with OS-specific one and remove "." and ".." from the path
char* sanitize_filename (char* filename)
{
   char *dirs[MAX_PATH_COMPONENTS];  int n=0;  dirs[n++]=filename;  bool isdir=true;
   char *dst=filename;
   for (char *src=filename;  *src;  )    // Copy filename from src to dst, removing "." and ".." path components
   {
//printf("%d:%s -> %d:%s\n", src-filename, src, dst-filename, dst);
     if (isdir)  // If it's a first letter of next path component
     {
       if (src[0]=='.'  &&  in_set0 (src[1], ALL_PATH_DELIMITERS))   // Skip "." path component
       {
         src += src[1]? 2:1;
         continue;
       }
       if (src[0]=='.'  &&  src[1]=='.'  &&  in_set0 (src[2], ALL_PATH_DELIMITERS))    // Remove path component preceding ".."
       {
         src += src[2]? 3:2;
         dst = dirs[n-1];
         n>1 && n--;
         continue;
       }
       dirs[n++] = dst;
       if (n==elements(dirs))   // prevent out-of-array-bounds
         n=1;
     }
     *dst  = (in_set(*src,UNSUPPORTED_PATH_DELIMITERS)? PATH_DELIMITER : *src);
     isdir = in_set(*dst,ALL_PATH_DELIMITERS);
     src++, dst++;
  }
  if (dst>filename && dst[-1]==PATH_DELIMITER)   // Remove trailing slash
    dst--;
  *dst = 0;
  return filename;
}

/* Checking code
char *ptr[] = {"abcde","abc\\de","abc/de","abc/./de","./abc/de","abc/de/.","abc/de/","abc/de/..","abc/../de","../abc/de","abc/../../de","a/bc/de/../..","a/bc/de/../../fg",0};
for (char **p=ptr; *p; p++)
{
  char a[12345]; strcpy(a,*p);
  printf("%s %s\n", *p, sanitize_filename (a));
}
*/


// ****************************************************************************
// Windows charset conversion routines ****************************************
// ****************************************************************************

#ifdef FREEARC_WIN
// Converts UTF-8 string to UTF-16
WCHAR *utf8_to_utf16 (const char *utf8, WCHAR *_utf16)
{
  WCHAR *utf16 = _utf16;
  do {
    BYTE c = utf8[0];   UINT c32;
         if (c<=0x7F)   c32 = c;
    else if (c<=0xBF)   c32 = '?';
    else if (c<=0xDF)   c32 = ((c&0x1F) << 6) +  (utf8[1]&0x3F),  utf8++;
    else if (c<=0xEF)   c32 = ((c&0x0F) <<12) + ((utf8[1]&0x3F) << 6) +  (utf8[2]&0x3F),  utf8+=2;
    else                c32 = ((c&0x0F) <<18) + ((utf8[1]&0x3F) <<12) + ((utf8[2]&0x3F) << 6) + (utf8[3]&0x3F),  utf8+=3;

    // Now c32 represents full 32-bit Unicode char
    if (c32 <= 0xFFFF)  *utf16++ = c32;
    else                c32-=0x10000, *utf16++ = c32/0x400 + 0xd800, *utf16++ = c32%0x400 + 0xdc00;

  } while (*utf8++);
  return _utf16;
}

// Converts UTF-16 string to UTF-8
char *utf16_to_utf8 (const WCHAR *utf16, char *_utf8)
{
  char *utf8 = _utf8;
  do {
    UINT c = utf16[0];
    if (0xd800<=c && c<=0xdbff && 0xdc00<=utf16[1] && utf16[1]<=0xdfff)
      c = (c - 0xd800)*0x400 + (UINT)(*++utf16 - 0xdc00) + 0x10000;

    // Now c represents full 32-bit Unicode char
         if (c<=0x7F)   *utf8++ = c;
    else if (c<=0x07FF) *utf8++ = 0xC0|(c>> 6)&0x1F,  *utf8++ = 0x80|(c>> 0)&0x3F;
    else if (c<=0xFFFF) *utf8++ = 0xE0|(c>>12)&0x0F,  *utf8++ = 0x80|(c>> 6)&0x3F,  *utf8++ = 0x80|(c>> 0)&0x3F;
    else                *utf8++ = 0xF0|(c>>18)&0x0F,  *utf8++ = 0x80|(c>>12)&0x3F,  *utf8++ = 0x80|(c>> 6)&0x3F,  *utf8++ = 0x80|(c>> 0)&0x3F;

  } while (*utf16++);
  return _utf8;
}

// Converts UTF-8 string to OEM
char *utf8_to_oem (const char *utf8, char *oem)
{
  WCHAR *utf16 = (WCHAR*) malloc_msg(MY_FILENAME_MAX*4);
  utf8_to_utf16 (utf8, utf16);
  CharToOemW (utf16, oem);
  free (utf16);
  return oem;
}

// Converts OEM string to UTF-8
char *oem_to_utf8 (const char  *oem, char *utf8)
{
  WCHAR *utf16 = (WCHAR*) malloc_msg(MY_FILENAME_MAX*4);
  OemToCharW (oem, utf16);
  utf16_to_utf8 (utf16, utf8);
  free (utf16);
  return utf8;
}
#endif


//*****************************************************************************
// File/system operations *****************************************************
//*****************************************************************************

// Directory for temporary files
static CFILENAME TempDir = 0;
#if 0
// Set temporary files directory
void SetTempDir (CFILENAME dir)
{
  if (dir && TempDir && _tcscmp(dir,TempDir)==0)
    return;  // the same string
  FreeAndNil(TempDir);
  if (dir && *dir)
  {
    TempDir = (CFILENAME) malloc_msg ((_tcslen(dir)+1) * sizeof(*dir));
    _tcscpy (TempDir, dir);
  }
}

// Return last value set or OS-default temporary directory
CFILENAME GetTempDir (void)
{
  if (!TempDir)
  {
#ifdef FREEARC_WIN
    TempDir = (CFILENAME) malloc_msg();
    GetTempPathW(MY_FILENAME_MAX, TempDir);
    realloc (TempDir, (_tcslen(TempDir)+1) * sizeof(*TempDir));
#else
    TempDir = tempnam(NULL,NULL);
    CFILENAME basename = drop_dirname(TempDir);
    if (basename > TempDir)
      basename[-1] = '\0';
#endif
  }
  return TempDir;
}


#ifdef FREEARC_WIN
#include <sys/utime.h>

#if !defined(_WIN64) && defined(__GNUC__)

typedef struct _MY_MEMORYSTATUSEX {
  DWORD dwLength;
  DWORD dwMemoryLoad;
  DWORDLONG ullTotalPhys;
  DWORDLONG ullAvailPhys;
  DWORDLONG ullTotalPageFile;
  DWORDLONG ullAvailPageFile;
  DWORDLONG ullTotalVirtual;
  DWORDLONG ullAvailVirtual;
  DWORDLONG ullAvailExtendedVirtual;
} MY_MEMORYSTATUSEX, *MY_LPMEMORYSTATUSEX;

#else

#define MY_MEMORYSTATUSEX MEMORYSTATUSEX
#define MY_LPMEMORYSTATUSEX LPMEMORYSTATUSEX

#endif

typedef BOOL (WINAPI *GlobalMemoryStatusExP)(MY_LPMEMORYSTATUSEX lpBuffer);

uint64 GetPhysicalMemory (void)
{
  MY_MEMORYSTATUSEX statx;
  statx.dwLength = sizeof(statx);
  GlobalMemoryStatusExP globalMemoryStatusEx = (GlobalMemoryStatusExP) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GlobalMemoryStatusEx");
  if (globalMemoryStatusEx != 0 && globalMemoryStatusEx(&statx))
    return statx.ullTotalPhys;

  MEMORYSTATUS stat;
  stat.dwLength = sizeof(stat);
  GlobalMemoryStatus(&stat);
  return stat.dwTotalPhys;
}

uint64 GetAvailablePhysicalMemory (void)
{
  MY_MEMORYSTATUSEX statx;
  statx.dwLength = sizeof(statx);
  GlobalMemoryStatusExP globalMemoryStatusEx = (GlobalMemoryStatusExP) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GlobalMemoryStatusEx");
  if (globalMemoryStatusEx != 0 && globalMemoryStatusEx(&statx))
    return statx.ullAvailPhys;

  MEMORYSTATUS stat;
  stat.dwLength = sizeof(stat);
  GlobalMemoryStatus(&stat);
  return stat.dwAvailPhys;
}

int GetProcessorsCount (void)
{
  SYSTEM_INFO si;
    GetSystemInfo (&si);
  return si.dwNumberOfProcessors;
}

void SetFileDateTime (CFILENAME Filename, time_t t)
{
  if (t<0)  t=INT_MAX;  // хМЮВЕ ОНКСВЮЕЛ БШКЕР :(
  struct tm* t2 = gmtime(&t);

  SYSTEMTIME t3;
  t3.wYear         = t2->tm_year+1900;
  t3.wMonth        = t2->tm_mon+1;
  t3.wDay          = t2->tm_mday;
  t3.wHour         = t2->tm_hour;
  t3.wMinute       = t2->tm_min;
  t3.wSecond       = t2->tm_sec;
  t3.wMilliseconds = 0;

  FILETIME ft;
  SystemTimeToFileTime(&t3, &ft);

  HANDLE hndl = CreateFileW(Filename, FILE_WRITE_ATTRIBUTES,
                            FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
  SetFileTime(hndl,NULL,NULL,&ft);  //creation, last access, modification times
  CloseHandle(hndl);
  //SetFileAttributes (Filename, attrib);
}

// Execute program `filename` in the directory `curdir` optionally waiting until it finished
void RunProgram (CFILENAME filename, CFILENAME curdir, int wait_finish)
{
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory (&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory (&pi, sizeof(pi));
  BOOL process_created = CreateProcessW (filename, NULL, NULL, NULL, FALSE, 0, NULL, curdir, &si, &pi);

  if (process_created)
  {
    if (wait_finish)
      WaitForSingleObject (pi.hProcess, INFINITE);
    CloseHandle (pi.hProcess);
    CloseHandle (pi.hThread);
  }
}

// Execute `command` in the directory `curdir` optionally waiting until it finished
int RunCommand (CFILENAME command, CFILENAME curdir, int wait_finish, SIMPLE_CALLBACK *callback, void *auxdata)
{
  STARTUPINFO si;
  ZeroMemory (&si, sizeof(si));
  si.cb = sizeof(si);
  si.dwFlags = (callback? STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES : 0);  // `callback` used here as a flag distinguishing calls from C_External.cpp
  si.wShowWindow = SW_HIDE;  // For GUI executables - hide console window of called external decompressor; for console executables this flag is ignored anyway
  si.hStdInput   = (HANDLE) _get_osfhandle(0);
  si.hStdOutput  = (HANDLE) _get_osfhandle(1);
  si.hStdError   = (HANDLE) _get_osfhandle(2);
  PROCESS_INFORMATION pi;
  ZeroMemory (&pi, sizeof(pi));
  DWORD ExitCode = 0;  // ЙНД БНГБПЮРЮ БШГШБЮЕЛНИ ОПНЦПЮЛЛШ

  BOOL process_created = CreateProcessW (NULL, command, NULL, NULL, TRUE, 0, NULL, curdir, &si, &pi);
  if (callback)
    callback(auxdata);    // Action that should be executed BEFORE waiting for process finish
  if (process_created)
  {
    if (wait_finish)
      WaitForSingleObject (pi.hProcess, INFINITE),
      GetExitCodeProcess  (pi.hProcess, &ExitCode);
    CloseHandle (pi.hProcess);
    CloseHandle (pi.hThread);
  }
  return ExitCode;
}

// Execute file `filename` in the directory `curdir` optionally waiting until it finished
void RunFile (CFILENAME filename, CFILENAME curdir, int wait_finish)
{
  SHELLEXECUTEINFO sei;
  ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));
  sei.cbSize = sizeof(SHELLEXECUTEINFO);
  sei.fMask = (wait_finish? SEE_MASK_NOCLOSEPROCESS : 0);
  sei.hwnd = GetActiveWindow();
  sei.lpFile = filename;
  sei.lpDirectory = curdir;
  sei.nShow = SW_SHOW;

  CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
  DWORD rc = ShellExecuteEx(&sei);
  if (rc && wait_finish)
    WaitForSingleObject(sei.hProcess, INFINITE),
    CloseHandle (sei.hProcess);
}

// бЕПЯХЪ Windows
#define CheckWinVersion(ver)  (GetWinVersion() >= ver)

#define WIN_VERSION_VISTA 0x600
int GetWinVersion()
{
   static int result = -1;
   if (result < 0)
   {
     OSVERSIONINFOEX osvi;
     ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
     osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
     if( !GetVersionEx ((OSVERSIONINFO *) &osvi) )
        result = 0;
     else result = (VER_PLATFORM_WIN32_NT==osvi.dwPlatformId? osvi.dwMajorVersion*0x100+osvi.dwMinorVersion : 0);
   }
   return result;
}


#ifndef THREAD_MODE_BACKGROUND_BEGIN
#define THREAD_MODE_BACKGROUND_BEGIN    0x00010000
#define THREAD_MODE_BACKGROUND_END      0x00020000
#endif

// сЯРЮМНБХРЭ ОПХНПХРЕР РПЕДЮ ЙЮЙНИ ОНКЮЦЮЕРЯЪ ДКЪ РПЕДНБ ЯФЮРХЪ (ПЮЯОЮЙНБЙХ, ЬХТПНБЮМХЪ...).
// хЯОНКЭГСЕРЯЪ ДКЪ РПЕДНБ, ЙНРНПШЕ БШОНКМЪЧР РНКЭЙН ЯФЮРХЕ
void SetCompressionThreadPriority (void)
{
   SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
}

// бПЕЛЕММН СЯРЮМНБХРЭ ОПХНПХРЕР РПЕДЮ ЙЮЙНИ ОНКЮЦЮЕРЯЪ ДКЪ РПЕДНБ ЯФЮРХЪ (ПЮЯОЮЙНБЙХ, ЬХТПНБЮМХЪ...)
// Х БНГБПЮРХРЭ ЯРЮПНЕ ГМЮВЕМХЕ СПНБМЪ ОПХНПХРЕРЮ. хЯОНКЭГСЕРЯЪ ДКЪ РПЕДНБ, БПЕЛЕММН БШОНКМЪЧЫХУ ГЮДЮВХ ЯФЮРХЪ
int BeginCompressionThreadPriority (void)
{
   DWORD dwThreadPriority = GetThreadPriority(GetCurrentThread());
   SetCompressionThreadPriority();
   return dwThreadPriority;
}

// бНЯЯРЮМНБХРЭ ОПХНПХРЕР РПЕДЮ РЮЙХЛ, ЙЮЙ ЛШ ЕЦН ГЮОНЛМХКХ
void EndCompressionThreadPriority (int old_priority)
{
   SetThreadPriority (GetCurrentThread(), old_priority);
}


#else // For Unix:  ========================================================================================================================

#include <unistd.h>
#include <sys/resource.h>

uint64 GetPhysicalMemory (void)
{
  return uint64(sysconf(_SC_PHYS_PAGES)) * sysconf(_SC_PAGE_SIZE);
}

uint64 GetAvailablePhysicalMemory (void)
{
  return uint64(sysconf(_SC_AVPHYS_PAGES)) * sysconf(_SC_PAGE_SIZE);
}

int GetProcessorsCount (void)
{
  return sysconf(_SC_NPROCESSORS_ONLN);
}

void SetFileDateTime(CFILENAME Filename, time_t t)
{
  if (t<0)  t=INT_MAX;  // хМЮВЕ ОНКСВЮЕЛ БШКЕР :(
  struct stat st;
    stat (Filename, &st);
  struct utimbuf times;
    times.actime  = st.st_atime;
    times.modtime = t;
  utime (Filename, &times);
}

// Execute `command` in the directory `curdir` optionally waiting until it finished
int RunCommand (CFILENAME command, CFILENAME curdir, int wait_finish, SIMPLE_CALLBACK *callback, void *auxdata)
{
  char *olddir = (char*) malloc_msg (),
       *cmd    = (char*) malloc_msg (strlen(command)+10);
  getcwd(olddir, MY_FILENAME_MAX);

  chdir(curdir);
  const char *prefix = (memcmp(command,"/"  ,1) == 0)?    "" :
                       (memcmp(command,"\"/",2) == 0)?    "" :
                       (memcmp(command,"\"" ,1) == 0)?    (command++, "\"./") :
                                                          "./";
  sprintf(cmd, "%s%s%s", prefix, command, wait_finish? "" : " &");
  int ExitCode = system(cmd);

  if (callback)
    callback(auxdata);    // Action that should be executed BEFORE waiting for process finish

  chdir(olddir);
  free(cmd);
  free(olddir);
  return ExitCode;
}

// Execute file `filename` in the directory `curdir` optionally waiting until it finished
void RunFile (CFILENAME filename, CFILENAME curdir, int wait_finish)
{
  RunCommand (filename, curdir, wait_finish, NULL, NULL);
}

// сЯРЮМНБХРЭ ОПХНПХРЕР РПЕДЮ ЙЮЙНИ ОНКЮЦЮЕРЯЪ ДКЪ РПЕДНБ ЯФЮРХЪ (ПЮЯОЮЙНБЙХ, ЬХТПНБЮМХЪ...).
// хЯОНКЭГСЕРЯЪ ДКЪ РПЕДНБ, ЙНРНПШЕ БШОНКМЪЧР РНКЭЙН ЯФЮРХЕ
void SetCompressionThreadPriority (void)
{
  int old = getpriority(PRIO_PROCESS, 0);
  setpriority(PRIO_PROCESS, 0, old+1);
}

// бПЕЛЕММН СЯРЮМНБХРЭ ОПХНПХРЕР РПЕДЮ ЙЮЙНИ ОНКЮЦЮЕРЯЪ ДКЪ РПЕДНБ ЯФЮРХЪ (ПЮЯОЮЙНБЙХ, ЬХТПНБЮМХЪ...)
int BeginCompressionThreadPriority (void)
{
  int old = getpriority(PRIO_PROCESS, 0);
  //setpriority(PRIO_PROCESS, 0, old+1);        ГЮЙНЛЛЕМРХПНБЮМН ХГ-ГЮ ОПНАКЕЛ Я БНЯЯРЮМНБКЕМХЕЛ ЯРЮПНЦН ОПХНПХРЕРЮ :(
  return old;
}

// бНЯЯРЮМНБХРЭ ОПХНПХРЕР РПЕДЮ РЮЙХЛ, ЙЮЙ ЛШ ЕЦН ГЮОНЛМХКХ
void EndCompressionThreadPriority (int old_priority)
{
  //setpriority(PRIO_PROCESS, 0, old_priority);
}

#endif // Windows/Unix


// яНГДЮРЭ ЙЮРЮКНЦХ МЮ ОСРХ Й name
void BuildPathTo (CFILENAME name)
{
  CFILENAME path_ptr = NULL;
  for (CFILENAME p = _tcschr(name,0); --p >= name;)
    if (_tcschr (_T(DIRECTORY_DELIMITERS), *p))
      {path_ptr=p; break;}
  if (path_ptr==NULL)  return;

  TCHAR oldc = *path_ptr;
  *path_ptr = 0;

  if (! file_exists (name))
  {
    BuildPathTo (name);
    create_dir  (name);
  }
  *path_ptr = oldc;
}


// ****************************************************************************************************************************
// онддепфйю яохяйю бпелеммшу тюикнб х сдюкемхе ху опх юбюпхимнл бшунде хг опнцпюллш ******************************************
// ****************************************************************************************************************************

// Table of temporary files that should be deleted on ^Break
static int TemporaryFilesCount=0;
static MYFILE *TemporaryFiles[100];

void registerTemporaryFile (MYFILE &file)
{
  unregisterTemporaryFile (file);  // First, delete all existing registrations of the same file
  TemporaryFiles[TemporaryFilesCount] = &file;
  if (TemporaryFilesCount < elements(TemporaryFiles))
    TemporaryFilesCount++;
}

void unregisterTemporaryFile (MYFILE &file)
{
  iterate_var(i,TemporaryFilesCount)
    if (TemporaryFiles[i] == &file)
    {
      memmove (TemporaryFiles+i, TemporaryFiles+i+1, (TemporaryFilesCount-(i+1)) * sizeof(TemporaryFiles[i]));
      TemporaryFilesCount--;
      return;
    }
}

void removeTemporaryFiles (void)
{
  // Enum files in reverse order in order to delete dirs after files they contain
  for(int i=TemporaryFilesCount-1; i>=0; i--)
    TemporaryFiles[i]->tryClose(),
    TemporaryFiles[i]->remove();
}



#ifndef FREEARC_NO_TIMING

//*****************************************************************************
// бШБНД ГЮЦНКНБЙЮ НЙМЮ *******************************************************
//*****************************************************************************

#ifdef FREEARC_WIN
#include <windows.h>

TCHAR Saved_Title[MY_FILENAME_MAX+1000];
bool Saved = FALSE,  SavedA = FALSE;

// сЯРЮМНБХРЭ ГЮЦНКНБНЙ ЙНМЯНКЭМНЦН НЙМЮ
void EnvSetConsoleTitle (TCHAR *title)
{
  if (!Saved && !SavedA) {
    GetConsoleTitle (Saved_Title, sizeof(Saved_Title)/sizeof(*Saved_Title));
    Saved = TRUE;
  }
  SetConsoleTitle (title);
}
void EnvSetConsoleTitleA (char *title)
{
  if (!Saved && !SavedA) {
    GetConsoleTitleA ((char*)Saved_Title, sizeof(Saved_Title));
    SavedA = TRUE;
  }
  SetConsoleTitleA (title);
}

// бНЯЯРЮМНБХРЭ ГЮЦНКНБНЙ, ЙНРНПШИ АШК Б МЮВЮКЕ ПЮАНРШ ОПНЦПЮЛЛШ
void EnvResetConsoleTitle (void)
{
  if (Saved) {
    SetConsoleTitle (Saved_Title);
    Saved = FALSE;
  }
  if (SavedA) {
    SetConsoleTitleA ((char*)Saved_Title);
    SavedA = FALSE;
  }
}

#else // !FREEARC_WIN

void EnvSetConsoleTitle (char *title)
{
  //Commented out since 1) we can't restore title on exit and 2) it looks unusual on Linux
  //  fprintf (stderr, "\033]0;%s\a", title);
}
void EnvSetConsoleTitleA (char *title)
{
  // See EnvSetConsoleTitle() definition
}

void EnvResetConsoleTitle (void)    {};

#endif


//*****************************************************************************
// Timing execution ***********************************************************
//*****************************************************************************

#ifdef FREEARC_WIN
// Returns number of wall-clock seconds since some moment
double GetGlobalTime (void)
{
  __int64 freq, t0;
  if( QueryPerformanceFrequency ((LARGE_INTEGER*)&freq) ) {
    QueryPerformanceCounter ((LARGE_INTEGER*)&t0);
    return ((double)t0)/freq;
  } else {
    return ((double)GetTickCount())/1000;
  }
}

// Returns number of seconds spent in this process
double GetCPUTime (void)
{
  FILETIME kt, ut, x, y;
  int ok = GetProcessTimes(GetCurrentProcess(),&x,&y,&kt,&ut);
  return !ok? -1 : ((double) (((long long)(ut.dwHighDateTime) << 32) + ut.dwLowDateTime)) / 10000000;
}

// Returns number of seconds spent in this thread
double GetThreadCPUTime (void)
{
  FILETIME kt, ut, x, y;
  int ok = GetThreadTimes(GetCurrentThread(),&x,&y,&kt,&ut);
  return !ok? -1 : ((double) (((long long)(ut.dwHighDateTime) << 32) + ut.dwLowDateTime)) / 10000000;
}
#endif // FREEARC_WIN


#ifdef FREEARC_UNIX
#include <sys/time.h>
#include <sys/resource.h>
// Returns number of wall-clock seconds since some moment
double GetGlobalTime (void)
{
  struct timespec ts;
  int res = clock_gettime(CLOCK_MONOTONIC, &ts);
  return res? -1 : (ts.tv_sec + ((double)ts.tv_nsec) / 1000000000);
}

// Returns number of seconds spent in this process
double GetCPUTime (void)
{
  struct rusage usage;
  int res = getrusage (RUSAGE_SELF, &usage);
  return res? -1 : (usage.ru_utime.tv_sec + ((double)usage.ru_utime.tv_usec) / 1000000);
}

// Returns number of seconds spent in this thread
double GetThreadCPUTime (void)
{
  struct rusage usage;
  int res = getrusage (RUSAGE_THREAD, &usage);
  return res? -1 : (usage.ru_utime.tv_sec + ((double)usage.ru_utime.tv_usec) / 1000000);
}
#endif // FREEARC_UNIX

// Time-based random number
unsigned time_based_random(void)
{
  double t = GetGlobalTime();
  return (unsigned)t + (unsigned)(t*1000000000);
}
#endif // !FREEARC_NO_TIMING


//*****************************************************************************
// Signal handler *************************************************************
//*****************************************************************************

#include <csignal>

void Install_signal_handler (void (__cdecl *signal_handler)(int))
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
#ifdef SIGBREAK
    signal(SIGBREAK, signal_handler);
#endif
}


//*****************************************************************************
// Windows 7 taskbar progress indicator ***************************************
//*****************************************************************************

#ifdef FREEARC_WIN

// Include Win7-specific defines unless on GCC3
#if !defined(__GNUC__) || __GNUC__>=4

#include <ShObjIdl.h>

#else  // hard-coded ShObjIdl.h definitions for the GCC3

#define MY_DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) EXTERN_C const GUID DECLSPEC_SELECTANY name = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }
MY_DEFINE_GUID(CLSID_TaskbarList, 0x56fdf344, 0xfd6d, 0x11d0, 0x95,0x8a, 0x00,0x60,0x97,0xc9,0xa0,0x90);
MY_DEFINE_GUID(IID_ITaskbarList3, 0xea1afb91, 0x9e28, 0x4b86, 0x90,0xe9, 0x9e,0x9f,0x8a,0x5e,0xef,0xaf);

typedef enum TBPFLAG {
    TBPF_NOPROGRESS = 0x0,
    TBPF_INDETERMINATE = 0x1,
    TBPF_NORMAL = 0x2,
    TBPF_ERROR = 0x4,
    TBPF_PAUSED = 0x8
} TBPFLAG;

struct ITaskbarList : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE HrInit() = 0;
    virtual HRESULT STDMETHODCALLTYPE AddTab(HWND hwnd) = 0;
    virtual HRESULT STDMETHODCALLTYPE DeleteTab(HWND hwnd) = 0;
    virtual HRESULT STDMETHODCALLTYPE ActivateTab(HWND hwnd) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetActiveAlt(HWND hwnd) = 0;
};
struct ITaskbarList2 : public ITaskbarList {
    virtual HRESULT STDMETHODCALLTYPE MarkFullscreenWindow(HWND hwnd, WINBOOL fFullscreen) = 0;
};
struct ITaskbarList3 : public ITaskbarList2 {
    virtual HRESULT STDMETHODCALLTYPE SetProgressValue(HWND hwnd, ULONGLONG ullCompleted, ULONGLONG ullTotal) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetProgressState(HWND hwnd, TBPFLAG tbpFlags) = 0;
};

#endif



class Taskbar
{
private:
    ITaskbarList3* m_pITaskBarList3;
    bool m_bFailed, local;
    HWND DefaultWindow;
    TBPFLAG PreviousState;

    bool Init(HWND hWnd)
    {
        DefaultWindow = hWnd? hWnd : GetConsoleWindow();

        if (m_pITaskBarList3)
            return true;
        if (m_bFailed)
            return false;

        // Initialize COM for this thread...
        CoInitialize(NULL);

        CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_ITaskbarList3, (void **)&m_pITaskBarList3);

        if (m_pITaskBarList3)
        {
            PreviousState = TBPF_NORMAL;
            return true;
        }

        m_bFailed = true;
        CoUninitialize();
        return false;
    }

public:
    Taskbar()
    {
        m_pITaskBarList3 = NULL;
        m_bFailed = false;
        DefaultWindow = NULL;
        Init(DefaultWindow);
    }

    virtual ~Taskbar()
    {
        Done();
    }

    void Done()
    {
        m_bFailed = true;

        if (DefaultWindow)
        {
            SetProgressState(TBPF_NOPROGRESS);
            DefaultWindow = NULL;
        }

        if (m_pITaskBarList3)
        {
            m_pITaskBarList3->Release();
            m_pITaskBarList3 = NULL;
            CoUninitialize();
        }
    }

    void SetProgressState(TBPFLAG flag)
    {
        Init(DefaultWindow);
        SetProgressState(DefaultWindow, flag);
    }

    void RestorePreviousProgressState()
    {
        Init(DefaultWindow);
        SetProgressState(DefaultWindow, PreviousState);
    }

    void SetProgressState(HWND hWnd, TBPFLAG flag)
    {
        if (Init(hWnd))
        {
            if (flag != TBPF_PAUSED)
                PreviousState = flag;
            m_pITaskBarList3->SetProgressState(hWnd, flag);
        }
    }

    void SetProgressValue(ULONGLONG ullCompleted, ULONGLONG ullTotal)
    {
        Init(DefaultWindow);
        SetProgressValue(DefaultWindow, ullCompleted, ullTotal);
    }

    void SetProgressValue(HWND hWnd, ULONGLONG ullCompleted, ULONGLONG ullTotal)
    {
        if (Init(hWnd))
            m_pITaskBarList3->SetProgressValue(hWnd, ullCompleted, ullTotal);
    }
};

static struct Taskbar DefaultTaskbar;
void Taskbar_SetWindowProgressValue (HWND hWnd, uint64 ullCompleted, uint64 ullTotal)  {DefaultTaskbar.SetProgressValue(hWnd, ullCompleted, ullTotal);}
void Taskbar_SetProgressValue (uint64 ullCompleted, uint64 ullTotal)  {DefaultTaskbar.SetProgressValue(ullCompleted, ullTotal);}
void Taskbar_Normal()                                                 {DefaultTaskbar.SetProgressState(TBPF_NORMAL);}
void Taskbar_Error ()                                                 {DefaultTaskbar.SetProgressState(TBPF_ERROR);}
void Taskbar_Pause ()                                                 {DefaultTaskbar.SetProgressState(TBPF_PAUSED);}
void Taskbar_Resume()                                                 {DefaultTaskbar.RestorePreviousProgressState();}
void Taskbar_Done  ()                                                 {DefaultTaskbar.Done();}
HWND FindWindowHandleByTitle(char *title)                             {return FindWindowA(NULL,title);}


#else // !FREEARC_WIN

void Taskbar_SetProgressValue (uint64, uint64) {}
void Taskbar_Normal() {}
void Taskbar_Error () {}
void Taskbar_Pause () {}
void Taskbar_Resume() {}
void Taskbar_Done  () {}

#endif


// ****************************************************************************************************************************
// ENCRYPTION ROUTINES *****************************************************************************************************
// ****************************************************************************************************************************

// Fills buf with OS-provided random data
int systemRandomData (void *buf, int len)
{
  static bool initialized = false;
  if (len == 0)  return 0;

#ifdef FREEARC_WIN

  // Alternative to CryptGenRandom: http://blogs.msdn.com/b/michael_howard/archive/2005/01/14/353379.aspx
  static HMODULE hLib = NULL;
  static BOOLEAN (APIENTRY *RtlGenRandom)(void*, ULONG) = NULL;

  if (!initialized)
  {
    hLib = LoadLibraryA("ADVAPI32.DLL");
    if (hLib)  RtlGenRandom = (BOOLEAN (APIENTRY *)(void*,ULONG))GetProcAddress(hLib,"SystemFunction036");
    initialized = true;
  }

  if (RtlGenRandom)
    if(RtlGenRandom(buf,len))
      return len;

#else  // UNIX

  static int f = -1;

  if (!initialized)
  {
    f = open ("/dev/random", O_RDONLY);
    initialized = true;
  }

  if (f >= 0)
  {
    int bytes = read (f, buf, len);
    if (bytes > 0)
      return bytes;
  }

#endif

  return 0;
}
#endif