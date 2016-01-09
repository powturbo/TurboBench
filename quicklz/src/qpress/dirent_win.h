#ifndef DIRENT_H
#define DIRENT_H

#include <stdio.h>

struct dirent
{
  char d_name[FILENAME_MAX+1];
};

typedef struct DIR DIR;

DIR * opendir ( const char * dirname );
struct dirent * readdir ( DIR * dir );
struct dirent * readdir_wildcard ( DIR * dir, char *wildcard );
int closedir ( DIR * dir );
void rewinddir ( DIR * dir );

#endif // DIRENT_H
