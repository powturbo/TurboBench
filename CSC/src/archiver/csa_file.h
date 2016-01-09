#ifndef _CSA_FILE_H_
#define _CSA_FILE_H_
#include <csa_common.h>
// File types accepting UTF-8 filenames

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


#ifdef unix

class InputFile {
  FILE* in;

public:
  InputFile(): in(0) {}

  bool open(const char* filename) {
    in=fopen(filename, "rb");
    return in!=0;
  }

  // True if open
  bool isopen() {return in!=0;}

  // Return file position
  int64_t tell() {
    return ftello(in);
  }

  uint32_t read(char *buf, uint32_t size) {
      return fread(buf, 1, size, in);
  }

  // Set file position
  void seek(int64_t pos, int whence) {
    if (whence==SEEK_CUR) {
      whence=SEEK_SET;
      pos+=tell();
    }
    fseeko(in, pos, whence);
  }

  // Close file if open
  void close() {if (in) fclose(in); in = 0;}
  ~InputFile() {close();}
};

class OutputFile {
  FILE* out;
  string filename;
  bool dummy;
public:
  OutputFile(): out(0), dummy(false) {}

  // Return true if file is open
  bool isopen() {return out!=0;}

  // Open for append/update or create if needed.
  // If aes then encrypt with aes+eoff.
  bool open(const char* filename) {
    assert(!isopen());
    this->filename=filename;
    dummy = (this->filename == DUMMY_FILENAME);
    if (dummy) {
        out = (FILE *)1;
        return true;
    }
    out=fopen(filename, "rb+");
    if (!out) out=fopen(filename, "wb+");
    if (!out) perror(filename);
    if (out) fseeko(out, 0, SEEK_END);
    return isopen();
  }

  // Write bufp[0..size-1]
  void write(const char* bufp, int size) {
      if (!dummy) fwrite(bufp, 1, size, out);
  }

  // Write size bytes at offset
  void write(const char* bufp, int64_t pos, int size) {
    assert(isopen());
    if (!dummy) {
        fseeko(out, pos, SEEK_SET);
        write(bufp, size);
    }
  }

  // Seek to pos. whence is SEEK_SET, SEEK_CUR, or SEEK_END
  void seek(int64_t pos, int whence) {
    assert(isopen());
    if (!dummy) {
        fseeko(out, pos, whence);
    }
  }

  // return position
  int64_t tell() {
    assert(isopen());
    if (!dummy)
        return ftello(out);
    else
        return 0;
  }

  // Truncate file and move file pointer to end
  void truncate(int64_t newsize=0) {
    assert(isopen());
    seek(newsize, SEEK_SET);
    if (!dummy && ftruncate(fileno(out), newsize)) perror("ftruncate");
  }

  // Close file and set date if not 0. Set permissions if attr low byte is 'u'
  void close(int64_t date=0, int64_t attr=0) {
    if (dummy) {
        dummy = false;
        out = 0;
        return;
    }
    if (out) {
      fclose(out);
    }
    out=0;
    if (date>0) {
      struct utimbuf ub;
      ub.actime=time(NULL);
      ub.modtime=unix_time(date);
      utime(filename.c_str(), &ub);
    }
    if ((attr&255)=='u')
      chmod(filename.c_str(), attr>>8);
  }

  ~OutputFile() {close();}
};

#else  // Windows

class InputFile {
  HANDLE in;  // input file handle
public:
  InputFile():
    in(INVALID_HANDLE_VALUE) {}

  // Open for reading. Return true if successful.
  // Encrypt with aes+e if aes.
  bool open(const char* filename) {
    assert(in==INVALID_HANDLE_VALUE);
    std::wstring w=utow(filename, true);
    in=CreateFile(w.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    return in!=INVALID_HANDLE_VALUE;
  }

  bool isopen() {return in!=INVALID_HANDLE_VALUE;}

  // set file pointer
  void seek(int64_t pos, int whence) {
    if (whence==SEEK_SET) whence=FILE_BEGIN;
    else if (whence==SEEK_END) whence=FILE_END;
    else if (whence==SEEK_CUR) {
      whence=FILE_BEGIN;
      pos+=tell();
    }
    LONG offhigh=pos>>32;
    SetFilePointer(in, pos, &offhigh, whence);
  }

    uint32_t read(char *buf, uint32_t size) {
        DWORD size_read;
        ReadFile(in, (LPVOID)buf, size, &size_read, NULL);
        return size_read;
    }

  // get file pointer
  int64_t tell() {
    LONG offhigh=0;
    DWORD r=SetFilePointer(in, 0, &offhigh, FILE_CURRENT);
    return (int64_t(offhigh)<<32)+r;
  }

  // Close handle if open
  void close() {
    if (in!=INVALID_HANDLE_VALUE) {
      CloseHandle(in);
      in=INVALID_HANDLE_VALUE;
    }
  }
  ~InputFile() {close();}
};

class OutputFile {
  HANDLE out;          
  std::wstring filename;    
  bool dummy;
public:
  OutputFile(): out(INVALID_HANDLE_VALUE),dummy(false) {}

  // Return true if file is open
  bool isopen() {
    return out!=INVALID_HANDLE_VALUE;
  }

  // Open file ready to update or append, create if needed.
  // If aes then encrypt with aes+e.
  bool open(const char* filename_) {
    assert(!isopen());
    dummy = (filename_ == DUMMY_FILENAME);
    if (dummy) {
        out = (HANDLE)1;
        return true;
    }
    filename=utow(filename_, true);
    out=CreateFile(filename.c_str(), GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, 
        NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (out==INVALID_HANDLE_VALUE) {
        fprintf(stderr, "File open error %s\n", filename.c_str());
        DWORD x = GetLastError();
        printf("%d", x);
    } else {
      LONG hi=0;
      SetFilePointer(out, 0, &hi, FILE_END);
    }
    return isopen();
  }

  // Write bufp[0..size-1]
  void write(const char* buf, int size) {
        DWORD size_written;
        if (!dummy)
            WriteFile(out, (LPVOID)buf, size, &size_written, NULL);   
  }

  // Write size bytes at offset
  void write(const char* buf, int64_t pos, int size) {
    assert(isopen());
    if (dummy)
        return;
    if (pos!=tell()) seek(pos, SEEK_SET);
    write(buf, size);
  }

  // set file pointer
  void seek(int64_t pos, int whence) {
    if (whence==SEEK_SET) whence=FILE_BEGIN;
    else if (whence==SEEK_CUR) whence=FILE_CURRENT;
    else if (whence==SEEK_END) whence=FILE_END;
    LONG offhigh=pos>>32;
    if (!dummy)
        SetFilePointer(out, pos, &offhigh, whence);
  }

  // get file pointer
  int64_t tell() {
    LONG offhigh=0;
    if (dummy)
        return 0;
    DWORD r=SetFilePointer(out, 0, &offhigh, FILE_CURRENT);
    return (int64_t(offhigh)<<32)+r;
  }

  // Truncate file and move file pointer to end
  void truncate(int64_t newsize=0) {
    if (dummy)
        return;
    seek(newsize, SEEK_SET);
    SetEndOfFile(out);
  }

  // Close file and set date if not 0. Set attr if low byte is 'w'.
  void close(int64_t date=0, int64_t attr=0) {
    if (isopen()) {
        if (dummy) {
            out=INVALID_HANDLE_VALUE;
            dummy = false;
            return;
        }
      setDate(out, date);
      CloseHandle(out);
      out=INVALID_HANDLE_VALUE;
      if ((attr&255)=='w')
        SetFileAttributes(filename.c_str(), attr>>8);
      filename=L"";
    }
  }
  ~OutputFile() {close();}
};

#endif

void makepath(string path, int64_t date=0, int64_t attr=0);

#endif


