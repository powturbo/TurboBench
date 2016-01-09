#ifndef _CSA_WORKER_H_
#define _CSA_WORKER_H_
#include <csa_common.h>
#include <csa_typedef.h>
#include <csa_thread.h>
#include <csa_io.h>

class MainWorker {
protected:
    ThreadID thread_;
    Semaphore& sem_finish_;
    Semaphore got_task_;

    MainTask *task_;
    ArchiveBlocks* abs_;
    int last_ret_;

public:
    uint64_t processed_raw_;

private:
    bool finished_;

    static ThreadReturn entrance(void *arg) 
    {
        MainWorker *worker = (MainWorker *)arg; 
        return (ThreadReturn)worker->run();
    }

    virtual int do_work() = 0; 

    void *run() {
        while(1) {
            got_task_.wait();
            if (finished_)
                break;

            last_ret_ = do_work();
            processed_raw_ = 0;

            task_ = NULL;
            sem_finish_.signal();
        }
        return NULL;
    }

public:
    MainWorker(Semaphore &sem):
        sem_finish_(sem),
        task_(NULL),
        last_ret_(0),
        processed_raw_(0),
        finished_(false) {
            got_task_.init(0);
        }

    virtual ~MainWorker() {}

    void Run() {
        ::run(thread_, MainWorker::entrance, this);
    }

    void PutTask(MainTask& task, ArchiveBlocks& abs) {
        task_ = &task;
        abs_ = &abs;
        got_task_.signal();
    }

    bool TaskDone() {
        return task_ == NULL;
    }

    int LastReturn() {
        return last_ret_;
    }
    
    void Finish() {
        finished_ = true;
        got_task_.signal();
        ::join(thread_);
    }

    uint64_t GetProcessedRawSize() const {
        return processed_raw_;
    }
};

class CompressionWorker: public MainWorker {
    int level_;
    uint32_t dict_size_;
    Mutex& arc_lock_;
    int do_work();

    static int update_progress(void *p, UInt64 insize, UInt64 outsize) {
        //CompressionWorker *worker = (CompressionWorker *)p;
        //worker->processed_raw_ = insize;
        return 0;
    }

public:
    CompressionWorker(Semaphore &sem, Mutex& arc_lock, int level, int dict_size) :
        MainWorker(sem),
        level_(level),
        dict_size_(dict_size),
        arc_lock_(arc_lock)
    {
    }

};

class DecompressionWorker: public MainWorker {
    int do_work();

    static int update_progress(void *p, UInt64 insize, UInt64 outsize) {
        //DecompressionWorker *worker = (DecompressionWorker *)p;
        //worker->processed_raw_ = outsize;
        return 0;
    }

public:
    DecompressionWorker(Semaphore &sem) :
        MainWorker(sem)
    {}
};



#endif

