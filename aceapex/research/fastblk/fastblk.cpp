// fastblk.cpp - #F1 prototype of "aceapex --fast": the ACEAPEX container idea (independent blocks + block
// table + one thread pool) with the misa77 token engine (MIT, (c) 2026 Shreyas Ghildiyal,
// github.com/welcome-to-the-sunny-side/misa77, used unmodified from the lzbench tree) inside each block.
// Question: does block parallelism lift a misa77-class decoder past the single-core leaders (misa77,
// zxc) and past the internally threaded codecs of lzbench -I (kanzi), at ratio <= 45 %?
// Decode: thread t takes a contiguous range of blocks and decodes them in order straight into the output
// (misa77 may write a little past a block's end; the next block of the same range overwrites it); the last
// block of each range goes through a private buffer + memcpy so no thread writes into another's range.
// Build: g++ -O3 -march=native -std=c++20 -I<misa77>/include -o fastblk fastblk.cpp <misa77 objects> -lpthread
// Usage: fastblk file [block=262144] [level=1] [reps=9]
#include <misa77/misa77.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>
static double now(){ return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
int main(int argc,char** argv){
    if(argc<2){ fprintf(stderr,"usage: %s file [block=262144] [level=1] [reps=9]\n",argv[0]); return 1; }
    const size_t BS=argc>2?strtoull(argv[2],0,10):262144; const int LV=argc>3?atoi(argv[3]):1, R=argc>4?atoi(argv[4]):9;
    FILE* f=fopen(argv[1],"rb"); if(!f){ perror(argv[1]); return 1; } fseek(f,0,SEEK_END); size_t n=ftell(f); fseek(f,0,SEEK_SET);
    std::vector<uint8_t> in(n); if(fread(in.data(),1,n,f)!=n) return 1; fclose(f);
    const size_t nb=(n+BS-1)/BS; misa77::config cfg((int8_t)LV);
    std::vector<std::vector<uint8_t>> cb(nb);
    double tc=now();
    { std::vector<std::thread> th; const int T=16;
      for(int t=0;t<T;t++) th.emplace_back([&,t]{ for(size_t b=t;b<nb;b+=T){ size_t o=b*BS, k=std::min(BS,n-o);
          cb[b].resize(misa77::compress_bound(k,cfg)); size_t r=misa77::compress(in.data()+o,k,cb[b].data(),cb[b].size(),cfg); cb[b].resize(r); } });
      for(auto& x:th) x.join(); }
    tc=now()-tc;
    size_t csz=8*nb+8; for(auto& c:cb) csz+=c.size();          // + a u64 block table and a header word
    const size_t slack=misa77::decompressed_buffer_bound(BS)-BS;
    std::vector<uint8_t> out(n+slack+64);
    printf("%s: %zu B, %zu blocks of %zu, misa77 level %d: %zu B = %.2f %% (encode 16 threads %.2f s)\n",argv[1],n,nb,BS,LV,csz,100.0*csz/n,tc);
    for(int T:{1,2,4,8,16}){
        std::vector<double> ts; bool ok=true;
        for(int r=0;r<R;r++){
            memset(out.data(),0,n); double t0=now();
            std::vector<std::thread> th;
            for(int t=0;t<T;t++) th.emplace_back([&,t]{
                size_t b0=nb*t/T, b1=nb*(t+1)/T; std::vector<uint8_t> priv;
                for(size_t b=b0;b<b1;b++){ size_t o=b*BS, k=std::min(BS,n-o);
                    if(b+1==b1 && b+1<nb){ priv.resize(misa77::decompressed_buffer_bound(k));
                        if(misa77::decompress(cb[b].data(),cb[b].size(),priv.data(),priv.size())!=k) ok=false; memcpy(out.data()+o,priv.data(),k); }
                    else if(misa77::decompress(cb[b].data(),cb[b].size(),out.data()+o,n+slack-o)!=k) ok=false; } });
            for(auto& x:th) x.join(); ts.push_back(now()-t0);
        }
        ok=ok && memcmp(out.data(),in.data(),n)==0; std::sort(ts.begin(),ts.end()); double m=ts[ts.size()/2];
        printf("decode %2d thread(s): %.4f s = %6.0f MB/s  %s\n",T,m,n/m/1e6,ok?"bit-perfect":"MISMATCH");
    }
    return 0;
}
