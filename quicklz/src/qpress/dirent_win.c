// dirent.c: emulates POSIX directory readin functions: opendir(), readdir(),
//           etc. under Win32   
//

#include <windows.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <malloc.h>
#include "dirent_win.h"

struct DIR
{
  HANDLE hFind;
  char   szDirName[1];
};

//--------------------------------------------------------------------------
// Name         countslashes
//
// Description  
//--------------------------------------------------------------------------
static int countslashes(const char *dirname)
{
  const char *p;
  int n;

  n = 0;
  p = dirname;

  while (*p)
    if (*p++ == '\\')
      ++n;

  return n;
}

//--------------------------------------------------------------------------
// Name         opendir
//
// Description  
//--------------------------------------------------------------------------
DIR * opendir ( const char * dirname )
{
  DIR * dir;
  int   nameLen;
  struct stat st;
  unsigned char flagNetPath;
  unsigned char flagRootOnly;

  if (dirname == NULL || *dirname == 0)
  {
    errno = EINVAL;
    return NULL;
  }

  nameLen = strlen( dirname );
  flagNetPath = 0;
  if (dirname[0] == '\\' && dirname[1] == '\\')
    flagNetPath = 1;
  /* we have to check for root-dir-only case */
  flagRootOnly = 0;
  if (flagNetPath)
  {
    if (countslashes(&dirname[2]) == 2)  /* only the separation for server_name and the root*/
      flagRootOnly = 1;
  }

  if ((dirname[nameLen-1] == '/' || dirname[nameLen-1] == '\\') &&
      (nameLen != 3 || dirname[1] != ':') && nameLen != 1 && !flagRootOnly)
  {
    char * t = (char *)alloca( nameLen );
    memcpy( t, dirname, nameLen );
    t[nameLen-1] = 0;
    dirname = t;
    --nameLen;
  }

  if (stat( dirname, &st ))
    return NULL;

  if ((st.st_mode & S_IFDIR) == 0)
  {
    // this is not a DIR
    errno = ENOTDIR;
    return NULL;
  }

  if ((dir = (DIR *)malloc( sizeof( DIR ) + nameLen + 2 )) == NULL)
  {
    errno = ENOMEM;
    return NULL;
  }

  dir->hFind = INVALID_HANDLE_VALUE;

  memcpy( dir->szDirName, dirname, nameLen );
  if (nameLen && dirname[nameLen-1] != ':' && dirname[nameLen-1] != '\\' &&
      dirname[nameLen-1] != '/')
  {
    dir->szDirName[nameLen++] = '\\';
  }
  dir->szDirName[nameLen] = '*';
  dir->szDirName[nameLen+1] = 0;

  return dir;
};

//--------------------------------------------------------------------------
// Name         readdir
//
// Description  
//--------------------------------------------------------------------------
struct dirent * readdir ( DIR * dir )
{
  static WIN32_FIND_DATA fData;

  if (dir == NULL)
  {
    errno = EBADF;
    return NULL;
  }

//  do
  {
    int ok = 1;

    if (dir->hFind == INVALID_HANDLE_VALUE)
    {
      dir->hFind = FindFirstFile((char *)dir->szDirName, &fData );
      if (dir->hFind == INVALID_HANDLE_VALUE)
        ok = 0;
    }
    else
    if (!FindNextFile( dir->hFind, &fData ))
      ok = 0;

    if (!ok)
    {
      switch (GetLastError())
      {
        case ERROR_NO_MORE_FILES:
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
          errno = ENOENT;
          break;

        case ERROR_NOT_ENOUGH_MEMORY:
          errno = ENOMEM;
          break;

        default:
          errno = EINVAL;
          break;
      }
      return NULL;
    }
  }
//  while (fData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN);

  return (struct dirent *)&fData.cFileName;
};



struct dirent * readdir_wildcard ( DIR * dir, char *wildcard)
{
  static WIN32_FIND_DATA fData;
  char search[1024];
  memset(search, 0, 1024);
  strncpy(search, dir->szDirName, strlen(dir->szDirName) - 1);
  strcat(search, wildcard);


  if (dir == NULL)
  {
    errno = EBADF;
    return NULL;
  }

//  do
  {
    int ok = 1;

    if (dir->hFind == INVALID_HANDLE_VALUE)
    {
      dir->hFind = FindFirstFile(search, &fData );
      if (dir->hFind == INVALID_HANDLE_VALUE)
        ok = 0;
    }
    else
    if (!FindNextFile( dir->hFind, &fData ))
      ok = 0;

    if (!ok)
    {
      switch (GetLastError())
      {
        case ERROR_NO_MORE_FILES:
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
          errno = ENOENT;
          break;

        case ERROR_NOT_ENOUGH_MEMORY:
          errno = ENOMEM;
          break;

        default:
          errno = EINVAL;
          break;
      }
      return NULL;
    }
  }
//  while (fData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN);

  return (struct dirent *)&fData.cFileName;
};

//--------------------------------------------------------------------------
// Name         closedir
//
// Description  
//--------------------------------------------------------------------------
int closedir ( DIR * dir )
{
  if (dir == NULL)
  {
    errno = EBADF;
    return -1;
  }
  if (dir->hFind != INVALID_HANDLE_VALUE)
    FindClose( dir->hFind );
  free( dir );
  return 0;
};

//--------------------------------------------------------------------------
// Name         rewinddir
//
// Description  
//--------------------------------------------------------------------------
void rewinddir ( DIR * dir )
{
  if (dir)
  {
    if (dir->hFind != INVALID_HANDLE_VALUE)
      FindClose( dir->hFind );
    dir->hFind = INVALID_HANDLE_VALUE;
  }
};

