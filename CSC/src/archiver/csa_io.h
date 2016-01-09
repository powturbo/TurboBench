#ifndef _CSA_IO_
#define _CSA_IO_

#include <csa_file.h>
#include <csa_typedef.h>
#include <csa_thread.h>
#include <csa_adler32.h>
#include <deque>
#include <algorithm>

class AsyncReader;
class AsyncWriter;

class AsyncReader {
protected:
    ThreadID iothread_;
    static ThreadReturn entrance(void *arg) 
    {
        AsyncReader *ar = (AsyncReader *)arg; 
        return ar->run();
    }

    Mutex lock_;
    Semaphore sem_full_;
    Semaphore sem_empty_;


    struct Block {
        char *buf;
        uint32_t size;
        uint32_t prog;
    };

    std::deque<Block> queue_;
    uint32_t size_;
    uint32_t bufsize_;
    volatile bool finished_;

    virtual ThreadReturn run() = 0;

public:
    AsyncReader() :
    finished_(false)
    {
        init_mutex(lock_);
        sem_full_.init(0);
        sem_empty_.init(0);
    }

    virtual ~AsyncReader() {
        while(!queue_.empty()) {
            Block &b = queue_.front();
            delete[] b.buf;
            queue_.pop_front();
        }
        destroy_mutex(lock_);
        sem_full_.destroy();
        sem_empty_.destroy();
    }

    void Run() {
        ::run(iothread_, AsyncReader::entrance, this);
    }

    int Read(void *buf, size_t *size) {
        size_t prog = 0;
        for(; prog < *size;) {
            lock(lock_);
            if (size_ == 0) {
                release(lock_);
                if (finished_) {
                    *size = prog;
                    return 0;
                }
                sem_empty_.wait();
                continue;
            }

            Block &b = queue_.front();
            size_t cpy = std::min<uint64_t>(*size - prog, b.size - b.prog);
            memcpy((char *)buf + prog, b.buf + b.prog, cpy);
            prog += cpy;
            b.prog += cpy;
            if (b.prog == b.size) {
                delete[] b.buf;
                size_ -= b.size;
                queue_.pop_front();
                sem_full_.signal();
            }
            release(lock_);
        }
        return 0;
    }

    virtual void Finish() {
        lock(lock_);
        finished_ = true;
        release(lock_);
        sem_full_.signal();
        join(iothread_);
    }
};


class AsyncWriter {
protected:
    struct Block {
        char *buf;
        uint32_t size;
        uint32_t prog;
    };

private:
    Block curblock_;
    uint32_t cap_;

protected:
    ThreadID iothread_;
    Mutex lock_;
    Semaphore sem_full_;
    Semaphore sem_empty_;
    uint32_t size_;
    uint32_t bufsize_;
    volatile bool finished_;

    static ThreadReturn entrance(void *arg) 
    {
        AsyncWriter *ar = (AsyncWriter *)arg; 
        return ar->run();
    }

    std::deque<Block> queue_;
    virtual ThreadReturn run() = 0;

    void flush() {
        if (curblock_.size > 0) {
            queue_.push_back(curblock_);
            size_ += curblock_.size;
        } else {
            delete []curblock_.buf;
        }
        curblock_.buf = NULL;
        curblock_.size = 0;
    }

public:
    void Run() {
        ::run(iothread_, AsyncWriter::entrance, this);
    }

    AsyncWriter():
    finished_(false)
    {
        init_mutex(lock_);
        sem_full_.init(0);
        sem_empty_.init(0);
        curblock_.size = 0;
        cap_ = 1048576;
        curblock_.buf = new char[cap_];
    }

    virtual ~AsyncWriter() {
        while(!queue_.empty()) {
            Block &b = queue_.front();
            delete[] b.buf;
            queue_.pop_front();
        }
        delete[] curblock_.buf;
        destroy_mutex(lock_);
        sem_full_.destroy();
        sem_empty_.destroy();
    }

    size_t Write(const void *buf, size_t size) {
        for(;;) {
            lock(lock_);
            if (finished_ == true) {
                release(lock_);
                return CSC_WRITE_ABORT;
            }

            if (size_ >= bufsize_) {
                release(lock_);
                sem_full_.wait();
                continue;
            }

            if (curblock_.size + size > cap_) {
                flush();
                cap_ = std::max<uint32_t>(1048576, size);
                curblock_.buf = new char[cap_];
            }

            memcpy(curblock_.buf + curblock_.size, buf, size);
            curblock_.size += size;
            release(lock_);
            sem_empty_.signal();
            return size;
        }
    }

    virtual void Finish() {
    }

};

class AsyncFileReader : public AsyncReader {

    vector<FileBlock> &filelist_;
    size_t curfidx_;
    uint64_t curfprog_;
    uint64_t cumsize_;
    InputFile if_;

    ThreadReturn run() {
        while(1) {
            lock(lock_);
            if (size_ >= bufsize_) {
                release(lock_);
                sem_full_.wait();
                continue;
            }
            uint64_t rdsize = bufsize_ - size_;
            release(lock_);
            
            // open next file if last was done
            if (!if_.isopen()) {
                if (curfidx_ >= filelist_.size()) {
                    finished_ = true;
                    sem_empty_.signal();
                    break;
                }
                if (!if_.open(filelist_[curfidx_].filename.c_str())) {
                    filelist_[curfidx_].size = 0;
                    curfidx_++;
                    continue;
                }

                //printf("%s: %lld\n", filelist_[curfidx_].filename.c_str(), filelist_[curfidx_].size);
                filelist_[curfidx_].posblock = cumsize_;
                if_.seek(filelist_[curfidx_].off, SEEK_SET);
            }

            // how much to read?
            rdsize = std::min<uint64_t>(rdsize, filelist_[curfidx_].size - curfprog_);
            rdsize = std::min<uint64_t>(rdsize, 2 * 1048576);
            char *buf = new char[rdsize];
            // read and push to queue
            rdsize = if_.read(buf, rdsize);
            filelist_[curfidx_].checksum = adler32(filelist_[curfidx_].checksum, (const uint8_t *)buf, rdsize);
            Block b;
            b.prog = 0;
            b.size = rdsize;
            b.buf = buf;

            lock(lock_);
            queue_.push_back(b);
            size_ += b.size;
            release(lock_);
            sem_empty_.signal();

            curfprog_ += rdsize;
            cumsize_ += rdsize;
            // end of current file, close it
            if (curfprog_ == filelist_[curfidx_].size) {
                if_.close();
                curfprog_ = 0;
                curfidx_ ++;
            }
        }
        return (ThreadReturn)NULL;
    }

public:
    AsyncFileReader(vector<FileBlock> &filelist, uint32_t bufsize)
    : filelist_(filelist),
    curfidx_(0),
    curfprog_(0),
    cumsize_(0)
    {
        size_ = 0;
        bufsize_ = bufsize;
    }

    ~AsyncFileReader() {
    }
};

class AsyncFileWriter : public AsyncWriter {
    vector<FileBlock> &filelist_;
    uint64_t curfidx_;
    uint64_t curfprog_;
    uint64_t curbprog_;
    uint64_t cumsize_;
    bool done_;
    uint32_t curchecksum_;
 
    OutputFile of_;

    ThreadReturn run() {
        while(1) {
            lock(lock_);
            if (done_) {
                release(lock_);
                break;
            }

            if (size_ == 0) {
                if (finished_) {
                    release(lock_);
                    break;
                }
                release(lock_);
                sem_empty_.wait();
                continue;
            }

            Block& b = queue_.front();
            if (cumsize_ + b.size > filelist_[curfidx_].posblock) {
                if (!of_.isopen()) {
                    //printf("Opening %s\n", filelist_[curfidx_].filename.c_str());
                    if (!of_.open(filelist_[curfidx_].filename.c_str())) {
                        curfidx_++;
                        if (curfidx_ >= filelist_.size()) {
                            done_ = true;
                            finished_ = true;
                            sem_full_.signal();
                        }
                        release(lock_);
                        continue;
                    }
                    curfprog_ = 0;
                    of_.seek(filelist_[curfidx_].off, SEEK_SET);
                    curchecksum_ = 0;
                    // skip the unnecessary data in front of current file
                    if (cumsize_ + curbprog_ < filelist_[curfidx_].posblock)
                        curbprog_ += filelist_[curfidx_].posblock - cumsize_ - curbprog_;
                }
                
                uint64_t wrsize = std::min<uint64_t>(b.size - curbprog_, filelist_[curfidx_].size - curfprog_);
                of_.write(b.buf + curbprog_, wrsize);
                curchecksum_ = adler32(curchecksum_, (const uint8_t *)b.buf + curbprog_, wrsize);
                curfprog_ += wrsize;
                curbprog_ += wrsize;
                if (curfprog_ ==  filelist_[curfidx_].size) {
                    of_.close(filelist_[curfidx_].it->second.edate,
                        filelist_[curfidx_].it->second.eattr);
                    if (filelist_[curfidx_].checksum != curchecksum_)
                        fprintf(stderr, "******** %s extraction/verify failed\n", filelist_[curfidx_].it->first.c_str());

                    curfidx_++;
                    if (curfidx_ >= filelist_.size()) {
                        done_ = true;
                        finished_ = true;
                        sem_full_.signal();
                    }
                }
            } else {
                curbprog_ = b.size;
            }

            if (curbprog_ == b.size) {
                delete[] b.buf;
                size_ -= b.size;
                cumsize_ += b.size;
                queue_.pop_front();
                release(lock_);
                curbprog_ = 0;
                sem_full_.signal();
            } else {
                release(lock_);
            }
        }
        return NULL;
    }
 
public:

    AsyncFileWriter(vector<FileBlock> &filelist, uint32_t bufsize)
    : filelist_(filelist),
    curfidx_(0),
    curfprog_(0),
    curbprog_(0),
    cumsize_(0),
    done_(false)
    {
        size_ = 0,
        bufsize_ = bufsize;
    }

    ~AsyncFileWriter() {
    }

    void Finish() {
        lock(lock_);
        flush();
        finished_ = true;
        release(lock_);
        sem_empty_.signal();
        join(iothread_);
        if (of_.isopen()) {
            if (filelist_[curfidx_].checksum != curchecksum_)
                fprintf(stderr, "******** %s extraction/verify failed\n", filelist_[curfidx_].it->first.c_str());
            //filelist_[curfidx_].size = curfprog_;
            of_.close(filelist_[curfidx_].it->second.edate,
                    filelist_[curfidx_].it->second.eattr);
        }
    }
};


struct FileWriter {
    ISeqOutStream os;
    AsyncWriter *obj;
};

inline size_t file_write_proc(void *p, const void *buf, size_t size) 
{
    FileWriter *w = (FileWriter *)p;
    return w->obj->Write(buf, size);
}

struct FileReader {
    ISeqInStream is;
    AsyncReader *obj;
};

inline int file_read_proc(void *p, void *buf, size_t *size)
{
    FileReader *r = (FileReader *)p;
    return r->obj->Read(buf, size);
}

struct MemReader {
    ISeqInStream is;
    char *ptr;
    uint64_t size;
    uint64_t pos;
};

inline int mem_read_proc(void *p, void *buf, size_t *size)
{
    MemReader *r = (MemReader*)p;
    uint64_t s = std::min<uint64_t>(*size, r->size - r->pos);
    memcpy(buf, r->ptr + r->pos, s);
    r->pos += s;
    *size = s;
    return 0;
}

struct MemWriter {
    ISeqOutStream os;
    char *ptr;
    uint64_t size;
    uint64_t pos;
};

inline size_t mem_write_proc(void *p, const void *buf, size_t size) 
{
    MemWriter *r = (MemWriter*)p;
    uint64_t s = std::min<uint64_t>(size, r->size - r->pos);
    memcpy(r->ptr + r->pos, buf, s);
    r->pos += s;
    return s;
}



class AsyncArchiveReader;
class AsyncArchiveWriter;

class AsyncArchiveReader : public AsyncReader {
    ArchiveBlocks &ab_;
    size_t curbidx_;
    size_t curbprog_;
    InputFile if_;

    ThreadReturn run() {
        while(1) {
            lock(lock_);
            if (finished_) {
                release(lock_);
                break;
            }
            if (size_ >= bufsize_) {
                release(lock_);
                sem_full_.wait();
                continue;
            }
            uint64_t rdsize = bufsize_ - size_;
            release(lock_);
            
            // how much to read?
            rdsize = std::min<uint64_t>(rdsize, ab_.blocks[curbidx_].size - curbprog_);
            rdsize = std::min<uint64_t>(rdsize, 2 * 1048576);
            char *buf = new char[rdsize];
            // read and push to queue
            rdsize = if_.read(buf, rdsize);
            Block b;
            b.prog = 0;
            b.size = rdsize;
            b.buf = buf;

            lock(lock_);
            queue_.push_back(b);
            size_ += b.size;
            release(lock_);
            sem_empty_.signal();

            curbprog_ += rdsize;
            // end of current file, close it
            if (curbprog_ == ab_.blocks[curbidx_].size) {
                curbprog_ = 0;
                curbidx_ ++;
                if (curbidx_ < ab_.blocks.size())
                    if_.seek(ab_.blocks[curbidx_].off, SEEK_SET);
                else
                    break;
            }
        }
        return (ThreadReturn)NULL;
    }

public:
    AsyncArchiveReader(ArchiveBlocks &ab ,uint32_t bufsize)
    : ab_(ab),
    curbidx_(0),
    curbprog_(0)
    {
        size_ = 0,
        bufsize_ = bufsize;
        if_.open(ab_.filename.c_str());
        if_.seek(ab_.blocks[curbidx_].off, SEEK_SET);
    }

    ~AsyncArchiveReader() {
        if_.close();
    }
};

class AsyncArchiveWriter : public AsyncWriter {
    ArchiveBlocks &ab_;
    OutputFile of_;
    Mutex& arc_lock_;

    ThreadReturn run() {
        while(1) {
            lock(lock_);
            if (size_ == 0) {
                if (finished_) {
                    release(lock_);
                    break;
                }
                release(lock_);
                sem_empty_.wait();
                continue;
            }

            Block b = queue_.front();
            release(lock_);

            lock(arc_lock_);
            of_.seek(0, SEEK_END);
            uint64_t pos = of_.tell();
            of_.truncate(pos + b.size);
            release(arc_lock_);
            of_.write(b.buf, pos, b.size);
            delete[] b.buf;

            ArchiveBlocks::Block abb;
            abb.size = b.size;
            abb.off = pos;
            ab_.blocks.push_back(abb);

            lock(lock_);
            size_ -= b.size;
            queue_.pop_front();
            release(lock_);
            sem_full_.signal();
        }
        return (ThreadReturn)NULL;
    }
 
public:

    AsyncArchiveWriter(ArchiveBlocks &ab, uint32_t bufsize, Mutex& arc_lock)
    : ab_(ab),
    arc_lock_(arc_lock)
    {
        size_ = 0,
        bufsize_ = bufsize;
        of_.open(ab_.filename.c_str());
    }

    ~AsyncArchiveWriter() {
    }

    void Finish() {
        lock(lock_);
        flush();
        finished_ = true;
        release(lock_);
        sem_empty_.signal();
        join(iothread_);
        of_.close();
    }
};


#endif

