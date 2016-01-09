#include <csa_file.h>


void makepath(string path, int64_t date, int64_t attr) {
  int quiet = 1;
  for (size_t i = 0; i < path.size(); ++i) {
    if (path[i]=='\\' || path[i]=='/') {
      path[i]=0;
#ifdef unix
      int ok=!mkdir(path.c_str(), 0777);
#else
      int ok=CreateDirectory(utow(path.c_str(), true).c_str(), 0);
#endif
      if (ok && quiet<=0) {
          /*
        fprintf(con, "Created directory ");
        printUTF8(path.c_str(), con);
        fprintf(con, "\n");
        */
      }
      path[i]='/';
    }
  }

  // Set date and attributes
  string filename=path;
  if (filename!="" && filename[filename.size()-1]=='/') {
    filename=filename.substr(0, filename.size()-1);  // remove trailing slash
#ifdef unix
  if (date>0) {
    struct utimbuf ub;
    ub.actime=time(NULL);
    ub.modtime=unix_time(date);
    utime(filename.c_str(), &ub);
  }
  if ((attr&255)=='u')
    chmod(filename.c_str(), attr>>8);
#else
  for (int i=0; i<filename.size(); ++i)  // change to backslashes
    if (filename[i]=='/') filename[i]='\\';
  if (date>0) {
    HANDLE out=CreateFile(utow(filename.c_str(), true).c_str(),
                          FILE_WRITE_ATTRIBUTES, 0, NULL, OPEN_EXISTING,
                          FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (out!=INVALID_HANDLE_VALUE) {
      setDate(out, date);
      CloseHandle(out);
    }
    else {
        winError(filename.c_str());
    }
  }
  if ((attr&255)=='w') {
    SetFileAttributes(utow(filename.c_str(), true).c_str(), attr>>8);
  }
#endif
  }
}



