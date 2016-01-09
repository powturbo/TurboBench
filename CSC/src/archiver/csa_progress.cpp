#include <csa_progress.h>
#ifdef unix
#include <unistd.h>
#define my_sleep(k) usleep(k * 1000)
#else
#include <windows.h>
#define my_sleep(k) Sleep(k)
#endif
#include <stdio.h>

ThreadReturn ProgressIndicator::run()
{
    int k = 0;
    while(!finished_) {
        if (k == 0) {
            uint64_t done_size = 0;
            for(size_t i = 0; i < tasks_.size(); i++) {
                if (!tasks_[i].finished) continue;
                if (tasks_[i].filelist.rbegin()->posblock == 0)
                    done_size += tasks_[i].total_size;
                else {
                    const FileBlock& b = *tasks_[i].filelist.rbegin();
                    done_size += b.size + b.posblock;
                }
            }
            for(int i = 0; i < mt_count_; i++) 
                done_size += workers_[i]->GetProcessedRawSize();
            draw_progress_bar(done_size);
        }
        // refresh progress every 30 * 10 ms
        k = (k + 1) % 30;
        // check finish ever 10ms
        my_sleep(10);
    }
    draw_progress_bar(total_size_);
    printf("\n");
    return (ThreadReturn)NULL;
}

static uint32_t get_ratio(uint32_t base, uint64_t a, uint64_t b) {
    uint64_t p = b / base;
    if (p == 0) p = 1;
    a /= p;
    if (a > base)
        a = base;
    return a;
}

void ProgressIndicator::draw_progress_bar(uint64_t done_size) {
    char bar[53] = {0};
    bar[0] = '[';
    bar[51] = ']';
    uint32_t r = get_ratio(50, done_size, total_size_);
    for(uint32_t i = 1; i <= 50; i++) {
        if (i < r || r == 50)
            bar[i] = '=';
        else if (i == r )
            bar[i] = '>';
        else
            bar[i] = ' ';
    }
    char bs[100]; // 100 is long enough
    for(int i = 0; i < 100; i++)
        bs[i] = '\b';
    bs[99] = '\0';

    printf("%s%s  %2d%% done", bs, bar, get_ratio(100, done_size, total_size_));
    fflush(stdout);
}

