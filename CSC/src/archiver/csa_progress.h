#ifndef _CSA_PROGRESS_H_
#define _CSA_PROGRESS_H_

// This is a prograss indicator

#include <csa_thread.h>
#include <csa_worker.h>
#include <vector>
#include <csa_typedef.h>

class ProgressIndicator {
    ThreadID thread_;
    static ThreadReturn entrance(void *arg) 
    {
        ProgressIndicator *ar = (ProgressIndicator *)arg; 
        return ar->run();
    }
    
    ThreadReturn run();
    const vector<MainTask>& tasks_;
    const MainWorker **workers_;
    const int mt_count_;
    volatile bool finished_;
    uint64_t total_size_;
    void draw_progress_bar(uint64_t done_size);
        
public:
    ProgressIndicator(const vector<MainTask>& tasks,
            const MainWorker **workers, const int mt_count) :
        tasks_(tasks),
        workers_(workers),
        mt_count_(mt_count),
        finished_(false) 
    {
        total_size_ = 0;
        for(size_t i = 0; i < tasks_.size(); i++) {
            if (tasks[i].filelist.rbegin()->posblock == 0)
                total_size_ += tasks_[i].total_size;
            else {
                const FileBlock& b = *tasks_[i].filelist.rbegin();
                total_size_ += b.size + b.posblock;
            }
        }
    }

    void Run() {
        ::run(thread_, entrance, this);
    }

    void Finish() {
        finished_ = true;
        join(thread_);
    }
};

#endif

