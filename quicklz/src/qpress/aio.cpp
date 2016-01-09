/*
FILE_FLAG_NO_BUFFERING file I/O for Windows
Copyright Lasse Mikkel Reinhold 2006-2009
GPL-1 and GPL-2 licensed.

Only intended for use in qpress (too problem-specific design to have general usefullness).

FILE_FLAG_NO_BUFFERING file I/O on Windows requires source address, destination address and
transfer size to be sector aligned.

This library creates a read buffer (read_buffer), write buffer (write_buffer) and an aligned
intermediate transfer buffer (io_commit) and issues requests to the Windows API in multipla of 
AIO_MAX_SECTOR_SIZE in size.

The user first needs to call aio_init() with the largest transfer the user expects to take place
(in order to setup buffers).

On non-Windows all this is bypassed and translates into normal fread() and fwrite(). Same when
aio_init() is called with buffered_io = true.
*/

//#define PRE_ALLOCATE

#include "aio.hpp"
#include "utilities.hpp"

#define _CRT_SECURE_NO_WARNINGS

#include "utilities.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef WINDOWS
    #define handle_type HANDLE
    #include <windows.h>
#else
    #define handle_type FILE *
#endif

static size_t largest_request_pub;
static bool buffering = true;

// read
#ifdef WINDOWS
  static size_t avail;
  static size_t src;
#endif
static handle_type ifile;
static char *read_buffer = 0;

// write
static char *write_buffer = 0;
static char *io_commit = 0;

#ifdef WINDOWS
  static size_t queued = 0;
#endif

static handle_type ofile;
static unsigned long long written;
static char destination_file[5000];
static unsigned long long last_extended_to = 0;

bool aio_init(size_t largest_request, bool buffered_io)
{
    buffering = buffered_io;
    if(buffering == false)
    {
        size_t allocate = largest_request + AIO_MAX_SECTOR_SIZE - 1;
        largest_request_pub = largest_request;

        read_buffer = (char *)aligned_realloc(read_buffer, allocate, AIO_MAX_SECTOR_SIZE);
        io_commit = (char *)aligned_realloc(io_commit, allocate, AIO_MAX_SECTOR_SIZE);
        write_buffer = (char *)aligned_realloc(write_buffer, allocate, AIO_MAX_SECTOR_SIZE);

        if(read_buffer == 0 || io_commit == 0 || write_buffer == 0)
        {
            abort("Error allocating memory - decrease -T and -K flags");
        }
    }    
    return true;
}


size_t FREAD(void *DstBuf, size_t Count, handle_type File)
{    
#ifdef WINDOWS
    size_t cn = 0;
    DWORD read = 1;
    size_t n = Count;
    while(n > 0 && read != 0)
    {
        ReadFile(File, (char*)DstBuf + cn, (DWORD)n, &read, 0);
        cn += read;
        n = n - read;
    }
    return cn;
#else
    return fread(DstBuf, 1, Count, File);
#endif
}


size_t aread(void *dst, size_t size)
{  
#ifdef WINDOWS
    if(buffering)
    {
        return FREAD(dst, size, ifile);
    }
    else
    {
        char *dst2 = (char *)dst;
        if(size > largest_request_pub)
            abort("Internal error, aread(%d) with largest_request == %d", (int)size, (int)largest_request_pub);

        if(size > avail)
        {
            size_t pending = size;
            size_t n = (pending - avail) / AIO_MAX_SECTOR_SIZE;
            if(n * AIO_MAX_SECTOR_SIZE < pending - avail)
                n++;
            size_t read = FREAD(io_commit, n*AIO_MAX_SECTOR_SIZE, ifile);

            memcpy(dst2, read_buffer, avail);
            pending -= avail;
            dst2 += avail;

            if(pending > read)
            {
                size_t ret = read + avail;
                memcpy(dst2, io_commit, read);
                avail = 0;
                return ret;
            }
            else
            {
                memcpy(dst2, io_commit, pending);
                memcpy(read_buffer, io_commit + pending, read - pending);
                avail = read - pending;
                return size;
            }
        }
        else
        {
            memcpy(dst2, read_buffer, size);
            memmove(read_buffer, read_buffer + size, avail - size);
            avail -= size;
            return size;
        }
    }
#else
    return FREAD(dst, size, ifile);
#endif
}


void aclose_read(void)
{
#ifdef WINDOWS
    CloseHandle(ifile);
#else
    fclose(ifile);
#endif
}

bool aopen_read(const char *file)
{
#ifdef WINDOWS
    if(strcmp(file, "<stdin>") == 0)
    {
        ifile = GetStdHandle(STD_INPUT_HANDLE);
        return true;
    }
    else
    {
        if(buffering)
            ifile = CreateFile(file, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL, NULL);
        else
            ifile = CreateFile(file, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL, NULL);
        if (ifile != INVALID_HANDLE_VALUE)
        {
            src = 0;
            avail = 0;
            return true;
        }
        else
        {
            return false;
        }
    }
#else
    if(strcmp(file, "<stdin>") == 0)
    {
        ifile = stdin;
        return true;
    }
    else
    {
        ifile = fopen(file, "rb");
        return (ifile != 0);
    }
#endif
}


size_t FWRITE(const void *Str, size_t Count, handle_type File)
{
#ifdef WINDOWS
    DWORD dummy;
    WriteFile(File, Str, (DWORD)Count, &dummy, 0);
    return (size_t)dummy;
#else
    return fwrite(Str, 1, Count, File);
#endif
}


#ifdef WINDOWS
__int64 myFileSeek (HANDLE hf, __int64 distance, DWORD MoveMethod)
{
   LARGE_INTEGER li;
   li.QuadPart = distance;
   li.LowPart = SetFilePointer (hf, li.LowPart, &li.HighPart, MoveMethod);
   return li.QuadPart;
}
#endif

size_t awrite(const void *src, size_t size)
{   
#ifdef WINDOWS
    if(buffering)
    {    
#ifdef PRE_ALLOCATE
        if(written > last_extended_to)
        {
            myFileSeek(ofile, written + 1000000000, FILE_BEGIN);
            last_extended_to = written + 1000000000;
            SetEndOfFile(ofile);
            myFileSeek(ofile, written, FILE_BEGIN);
        }
#endif
        written += size;
        return FWRITE(src, size, ofile);
    }
    else
    {
        size_t n;

        if(size > largest_request_pub)
            abort("Internal error, aread(%d) with largest_request == %d", (int)size, (int)largest_request_pub);

        memcpy(write_buffer + queued, src, size);
        queued += size;
        written += size;
        n = queued / AIO_MAX_SECTOR_SIZE;
        size_t wrote = FWRITE(write_buffer, n*AIO_MAX_SECTOR_SIZE, ofile);
        queued -= n*AIO_MAX_SECTOR_SIZE;
        memmove(write_buffer, write_buffer + n*AIO_MAX_SECTOR_SIZE, queued);
        if (wrote != n*AIO_MAX_SECTOR_SIZE) 
            return 0;
        return size;
    }
#else
    written += size;
    return FWRITE(src, size, ofile);
#endif
}


unsigned long long awritten(void)
{
    return written;    
}


bool aclose_write(void)
{
#ifdef WINDOWS
    if(buffering)
    {
#ifdef PRE_ALLOCATE
        if(!CloseHandle(ofile))
            return false;
        ofile = CreateFile(destination_file, GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if(ofile == INVALID_HANDLE_VALUE)
            return false;
        myFileSeek(ofile, awritten(), SEEK_CUR);
        SetEndOfFile(ofile);  
#endif
        return CloseHandle(ofile) != 0;
    }
    else
    {
        if(ofile == GetStdHandle(STD_OUTPUT_HANDLE))
        {
            return CloseHandle(ofile) != 0;
        }
        else
        {
            // write remaining data in queue, round UP to nearest sector size (we may write
            // too much but file will be trucnated later, below).
            size_t n = (queued | (AIO_MAX_SECTOR_SIZE - 1)) + 1;
            FWRITE(write_buffer, n, ofile);

            if(!CloseHandle(ofile))
                return false;
            ofile = CreateFile(destination_file, GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if(ofile == INVALID_HANDLE_VALUE)
                return false;
            myFileSeek(ofile, awritten(), SEEK_CUR);
            SetEndOfFile(ofile);
            return CloseHandle(ofile) != 0;
        }
    }
#else
	if(ofile != 0)
	    fclose(ofile);
    return true;
#endif
}


bool aopen_write(const char *file)
{
    strcpy(destination_file, file);
    written = 0;
    last_extended_to = 0;

#ifdef WINDOWS
    if(strcmp(file, "<stdout>") == 0)
    {
        ofile = GetStdHandle(STD_OUTPUT_HANDLE);
        queued = 0;
        return true;
    }
    else
    {
        queued = 0;
        if(buffering)
            ofile = CreateFile(file, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL, NULL);
        else
            ofile = CreateFile(file, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL, NULL);
        return (ofile != INVALID_HANDLE_VALUE);
    }
#else
    if(strcmp(file, "<stdout>") == 0)
    {
        ofile = stdout;
        return true;
    }
    else
    {
        ofile = fopen(file, "wb");
        return (ofile != 0);
    }
#endif
}


size_t try_awrite(const void *Str, size_t Count)
{
    size_t r = awrite(Str, Count);
    if (r != Count)
        abort("Disk full while writing destination file");

    return r;
}

size_t try_aread(void *DstBuf, size_t Count)
{
    size_t r = aread(DstBuf, Count);
    if (r != Count)
        abort("Unexpected end of source file -- try the -R flag to recover");

    return r;
}

void fwrite64(unsigned long long i)
{
    unsigned long long j = i;
    char c[8];
    for(int k = 0; k < 8; k++)
    {
        c[k] = (unsigned char)j;
        j >>= 8;
    }
    try_awrite(c, 8);
}

void fwrite32(unsigned int i)
{
    unsigned int j = i;
    char c[4];
    for(int k = 0; k < 4; k++)
    {
        c[k] = (char)j;
        j >>= 8;
    }
    try_awrite(c, 4);
}

unsigned int fread32(void)
{
    unsigned int j = 0;
    unsigned char c[4];
    try_aread(c, 4);
    for(int k = 0; k < 4; k++)
    {
        j = j << 8;
        j = (j | c[3 - k]);
    }
    return j;
}

unsigned long long fread64()
{
    unsigned long long j = 0;
    unsigned char c[8];
    try_aread(c, 8);

    for(int k = 0; k < 8; k++)
    {
        j = j << 8;
        j = j | c[7 - k];
    }
    return j;
}

void adelete_write(void)
{
    aclose_write();
#ifdef WINDOWS
    DeleteFile(destination_file);
#else
    remove(destination_file);
#endif
}



