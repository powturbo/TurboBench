// gpu_api_test.cu - the C ABI of src/aceapex_gpu.h on a real GPU (Colab: scripts/colab_gpu_open.sh).
//   full     aceapex_gpu_decompress_async: output == original byte for byte, status 0; median time (events)
//   range    random windows (1 B .. 1 MiB) through aceapex_gpu_decompress_range_async, each == the original
//            slice (the file the CPU compressed); median time of the 16 KiB windows
//   verify   the same full decode with ACEAPEX_GPU_VERIFY_XXH3 (XXH3 of the output on the device against the
//            header): status 0 on the good archive, and the time it adds
//   corrupt  single-byte flips of the device copy of the archive, whole decode each, without and with the
//            hash check: counted as caught (status != 0), silent (status 0, output differs) or harmless (output
//            identical); a CUDA error (e.g. an illegal address) fails the test
// Progress: stdout unbuffered, a line per phase and per flip with the time since start ([+s]); every wait on
// the stream is a poll with a watchdog (AX_WATCHDOG seconds, default 300): past it the line TIMEOUT <phase>
// and exit 4 - a hang names its phase instead of stopping the run
// Flips: each one located (stream, zstd frame or rANS piece) and, in a zstd frame, the frame checked on the host first
// (libzstd, the plan's header check), saved to AX_REPRO (default verify/repro/run, original and flipped), decoded by
// nvCOMP alone (batch of 1) when libzstd decodes it, then the plan of the flipped archive - with ACEAPEX_GPU_VALIDATE_ZSTD
// when it has zstd frames: refused = caught before any launch - and the decode with that plan, a line and a stream wait
// per phase (aceapex_gpu_debug_phase_hook): a hang names the frame and the phase.
// Build: nvcc -O3 -arch=sm_XX -DACEAPEX_ENV_TUNING -Isrc -DACEAPEX_GPU_NVCOMP -I<nvcomp>/include scripts/gpu_api_test.cu src/aceapex_gpu_lib.cu src/aceapex_gpu_abi.cpp -l:libnvcomp.so.5 -lzstd
// Usage: gpu_api_test <archive.aet> <original> [repeats=7] [ranges=200] [flips=20]
// Last line: APIROW <tab> archive bytes api_ms full ranges_ok ranges range16k_ms caught silent harmless plan_ms
//            verify_ms verify_full caught_v silent_v harmless_v validate_plan_ms refused_by_plan tile_ms tile_check
#include "aceapex_gpu.h"
#include "aceapex_gpu_plan.h"   // host side only: where a flip lands (zstd frame, piece) and the frame header check
#include <cuda_runtime.h>
#include <nvcomp/zstd.h>
#include <zstd.h>
#include <string>
#include <sys/stat.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <vector>
#include <algorithm>
#include <thread>
#include <unistd.h>
#define CK(x) do{ cudaError_t e_=(x); if(e_!=cudaSuccess){ printf("CUDA %s @%d: %s\n",#x,__LINE__,cudaGetErrorString(e_)); exit(2);} }while(0)
static std::vector<uint8_t> slurp(const char* p){ FILE* f=fopen(p,"rb"); if(!f){ perror(p); exit(1);} fseek(f,0,SEEK_END); std::vector<uint8_t> v(ftell(f)); fseek(f,0,SEEK_SET);
    if(fread(v.data(),1,v.size(),f)!=v.size()) exit(1); fclose(f); return v; }
static const auto T0=std::chrono::steady_clock::now();
static double now_s(){ return std::chrono::duration<double>(std::chrono::steady_clock::now()-T0).count(); }
static void phase(const char* what){ printf("[+%.1f s] %s\n",now_s(),what); }
static void wait(cudaStream_t s, const char* what){                  // cudaStreamSynchronize with a watchdog
    static const double lim=getenv("AX_WATCHDOG")?atof(getenv("AX_WATCHDOG")):300; const double t=now_s();
    for(;;){ cudaError_t e=cudaStreamQuery(s); if(e==cudaSuccess) return;
        if(e!=cudaErrorNotReady){ printf("CUDA error in %s: %s\n",what,cudaGetErrorString(e)); exit(2); }
        if(now_s()-t>lim){ printf("TIMEOUT %s: the stream did not finish in %.0f s\n",what,lim); fflush(stdout); _exit(4); }
        std::this_thread::sleep_for(std::chrono::microseconds(200)); } }
extern "C" void aceapex_gpu_debug_phase_hook(void (*)(const char*, cudaStream_t));
static int g_flip=0; static const char* g_flipv="";
static void on_phase(const char* p, cudaStream_t s){
    std::string w=std::string("flip ")+std::to_string(g_flip)+g_flipv+": "+p; const double t=now_s(); wait(s,w.c_str());
    printf("[+%.1f s]   %s done (%.3f s)\n",now_s(),w.c_str(),now_s()-t); }
static uint64_t nvt_host(size_t n, size_t maxo, size_t tot){ size_t t=0;
    return nvcompBatchedZstdDecompressGetTempSizeAsync(n,maxo,nvcompBatchedZstdDecompressDefaultOpts,&t,tot)==nvcompSuccess ? t : ~0ull; }
static void put(const std::string& p, const uint8_t* b, size_t n){ FILE* f=fopen(p.c_str(),"wb"); if(f){ fwrite(b,1,n,f); fclose(f); } }
static float med(std::vector<float> v){ if(v.empty()) return -1; std::sort(v.begin(),v.end()); return v[v.size()/2]; }
int main(int argc, char** argv){
    if(argc<3){ fprintf(stderr,"usage: %s <archive.aet> <original> [repeats] [ranges] [flips]\n",argv[0]); return 1; }
    setvbuf(stdout,nullptr,_IONBF,0);
    const int reps=argc>3?atoi(argv[3]):7, NR=argc>4?atoi(argv[4]):200, NF=argc>5?atoi(argv[5]):20;
    phase("read archive and original"); std::vector<uint8_t> a=slurp(argv[1]), orig=slurp(argv[2]);
    phase("plan_create"); auto t0=std::chrono::steady_clock::now();
    aceapex_gpu_plan* plan=aceapex_gpu_plan_create(a.data(),a.size(),0);
    const double plan_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t0).count();
    double val_ms=-1;                                                      // the same with ACEAPEX_GPU_VALIDATE_ZSTD
    { phase("plan_create with ACEAPEX_GPU_VALIDATE_ZSTD"); auto tv=std::chrono::steady_clock::now(); aceapex_gpu_plan* q=aceapex_gpu_plan_create(a.data(),a.size(),ACEAPEX_GPU_VALIDATE_ZSTD);
      val_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-tv).count();
      // the first plan_create above also paid the CUDA context start (~160 ms): compare with a second plain one
      auto tw=std::chrono::steady_clock::now(); aceapex_gpu_plan* q0=aceapex_gpu_plan_create(a.data(),a.size(),0);
      const double warm_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-tw).count(); if(q0) aceapex_gpu_plan_destroy(q0);
      printf("[api] plan with ACEAPEX_GPU_VALIDATE_ZSTD %.1f ms (%+.1f ms against a plain plan of %.1f ms after the CUDA start: every zstd frame decoded by libzstd on %u host threads): %s\n",val_ms,val_ms-warm_ms,warm_ms,
             std::thread::hardware_concurrency(),q?"accepted":"REFUSED");
      if(q) aceapex_gpu_plan_destroy(q); }
    if(!plan){ printf("plan_create failed: %d\nAPIROW\t%s\t%zu\t-1\tNOPLAN\t0\t0\t-1\t0\t0\t0\t%.1f\n",aceapex_gpu_last_error(),argv[1],a.size(),plan_ms); return 3; }
    const size_t n=aceapex_gpu_output_bytes(plan); const uint64_t RMAX=std::min<uint64_t>(n,1u<<20);
    if(n!=orig.size()){ printf("output size %zu != original %zu\n",n,orig.size()); return 3; }
    const size_t tb=aceapex_gpu_range_temp_bytes(plan,RMAX);
    printf("[api] plan %.1f ms on the host; temp %.1f MB (full %.1f MB), output %.1f MB\n",plan_ms,tb/1e6,aceapex_gpu_temp_bytes(plan)/1e6,n/1e6);
    phase("allocate and upload"); uint8_t *d_in,*d_out,*d_temp,*d_bad; int* d_st; cudaStream_t s; CK(cudaStreamCreate(&s));
    CK(cudaMalloc(&d_in,a.size()+1)); CK(cudaMalloc(&d_bad,a.size()+1)); CK(cudaMalloc(&d_out,n+1)); CK(cudaMalloc(&d_temp,tb)); CK(cudaMalloc(&d_st,4));
    CK(cudaMemcpy(d_in,a.data(),a.size(),cudaMemcpyHostToDevice));
    std::vector<uint8_t> out(n); int st=-1;
    // full
    phase("full decode"); int r=aceapex_gpu_decompress_async(plan,d_in,d_out,d_temp,d_st,0,s);
    wait(s,"full decode"); CK(cudaGetLastError());
    CK(cudaMemcpy(&st,d_st,4,cudaMemcpyDeviceToHost)); CK(cudaMemcpy(out.data(),d_out,n,cudaMemcpyDeviceToHost));
    const bool full_ok = r==0 && st==0 && !memcmp(out.data(),orig.data(),n);
    printf("[api] full decode: return %d, status %d, output %s\n",r,st,full_ok?"MATCHES OK":"DIFFERS X");
    cudaEvent_t e0,e1; CK(cudaEventCreate(&e0)); CK(cudaEventCreate(&e1));
    std::vector<float> tf; phase("full decode timed");
    for(int i=0;i<reps;i++){ CK(cudaEventRecord(e0,s)); aceapex_gpu_decompress_async(plan,d_in,d_out,d_temp,d_st,0,s); CK(cudaEventRecord(e1,s)); wait(s,"full decode timed");
        float ms; CK(cudaEventElapsedTime(&ms,e0,e1)); tf.push_back(ms); }
    const float api_ms=med(tf);
    printf("[api] full decode on-device %.3f ms (median of %d) -> %.1f GB/s\n",api_ms,reps,n/api_ms/1e6);
    // the same with the XXH3 check of the output
    phase("full decode + XXH3"); CK(cudaMemset(d_out,0,n)); r=aceapex_gpu_decompress_async(plan,d_in,d_out,d_temp,d_st,ACEAPEX_GPU_VERIFY_XXH3,s); wait(s,"full decode + XXH3"); CK(cudaGetLastError());
    CK(cudaMemcpy(&st,d_st,4,cudaMemcpyDeviceToHost)); CK(cudaMemcpy(out.data(),d_out,n,cudaMemcpyDeviceToHost));
    const bool vfull_ok = r==0 && st==0 && !memcmp(out.data(),orig.data(),n);
    std::vector<float> tv; phase("full decode + XXH3 timed");
    for(int i=0;i<reps;i++){ CK(cudaEventRecord(e0,s)); aceapex_gpu_decompress_async(plan,d_in,d_out,d_temp,d_st,ACEAPEX_GPU_VERIFY_XXH3,s); CK(cudaEventRecord(e1,s)); wait(s,"full decode + XXH3 timed");
        float ms; CK(cudaEventElapsedTime(&ms,e0,e1)); tv.push_back(ms); }
    const float ver_ms=med(tv);
    printf("[api] full decode + XXH3 check on-device %.3f ms (median of %d; check adds %.3f ms, %+.1f %%): return %d, status %d, output %s\n",
        ver_ms,reps,ver_ms-api_ms,100.0*(ver_ms/api_ms-1),r,st,vfull_ok?"MATCHES OK":"DIFFERS X");
    // AX_GPU_TILE (library reads it at plan_create): a second plan, the full decode with literals in shared memory
    float tile_ms=-1; bool tile_ok=false;
    { setenv("AX_GPU_TILE","1",1); aceapex_gpu_plan* tp=aceapex_gpu_plan_create(a.data(),a.size(),0); unsetenv("AX_GPU_TILE");
      if(tp && aceapex_gpu_temp_bytes(tp)<=tb){ phase("full decode AX_GPU_TILE=1"); CK(cudaMemset(d_out,0,n));
          r=aceapex_gpu_decompress_async(tp,d_in,d_out,d_temp,d_st,0,s); wait(s,"full decode AX_GPU_TILE"); CK(cudaGetLastError());
          CK(cudaMemcpy(&st,d_st,4,cudaMemcpyDeviceToHost)); CK(cudaMemcpy(out.data(),d_out,n,cudaMemcpyDeviceToHost));
          tile_ok = r==0 && st==0 && !memcmp(out.data(),orig.data(),n);
          std::vector<float> tt; for(int i=0;i<reps;i++){ CK(cudaEventRecord(e0,s)); aceapex_gpu_decompress_async(tp,d_in,d_out,d_temp,d_st,0,s); CK(cudaEventRecord(e1,s)); wait(s,"full decode AX_GPU_TILE timed");
              float ms; CK(cudaEventElapsedTime(&ms,e0,e1)); tt.push_back(ms); }
          tile_ms=med(tt);
          printf("[api] AX_GPU_TILE=1 full decode on-device %.3f ms (default %.3f ms, %+.1f %%): return %d, status %d, output %s\n",tile_ms,api_ms,100.0*(tile_ms/api_ms-1),r,st,tile_ok?"MATCHES OK":"DIFFERS X"); }
      else printf("[api] AX_GPU_TILE=1: not applicable to this archive (open DNA packs / open plain pieces, 16 KiB blocks)\n");
      if(tp) aceapex_gpu_plan_destroy(tp); }
    // ranges
    phase("ranges"); std::mt19937_64 rng(20261001); int rok=0; std::vector<float> t16;
    const uint64_t lens[6]={1,17,4096,16384,65536,1u<<20};
    std::vector<uint8_t> rb(RMAX);
    for(int i=0;i<NR;i++){ uint64_t len=std::min<uint64_t>(lens[i%6],n), off=rng()%(n-len+1);
        CK(cudaEventRecord(e0,s)); r=aceapex_gpu_decompress_range_async(plan,d_in,off,len,d_out,d_temp,d_st,s); CK(cudaEventRecord(e1,s)); wait(s,"range");
        float ms; CK(cudaEventElapsedTime(&ms,e0,e1)); if(len==16384) t16.push_back(ms);
        CK(cudaMemcpy(&st,d_st,4,cudaMemcpyDeviceToHost)); CK(cudaMemcpy(rb.data(),d_out,len,cudaMemcpyDeviceToHost));
        if(r==0 && st==0 && !memcmp(rb.data(),orig.data()+off,len)) rok++; else if(rok>=0) printf("[api] range off=%llu len=%llu: return %d status %d DIFFERS X\n",(unsigned long long)off,(unsigned long long)len,r,st); }
    const float r16=med(t16);
    printf("[api] ranges: %d of %d windows == original%s; 16 KiB window %.3f ms (median of %zu)\n",rok,NR,rok==NR?" (MATCHES OK)":" DIFFERS X",r16,t16.size());
    // corruption
    int caught[2]={0,0}, silent[2]={0,0}, same[2]={0,0};
    agp::Plan HP; agp::build(a.data(),a.size(),HP,nvt_host);           // where flips land
    const char* RD=getenv("AX_REPRO")?getenv("AX_REPRO"):"verify/repro/run"; mkdir("verify/repro",0755); mkdir(RD,0755);
    std::string base=argv[1]; base=base.substr(base.find_last_of('/')+1);
    int refused=0;
    uint64_t sb[5]; { uint32_t nb; memcpy(&nb,a.data()+24,4); sb[0]=68+64ull*nb; for(int k=0;k<4;k++){ uint64_t z; memcpy(&z,a.data()+36+8*k,8); sb[k+1]=sb[k]+z; } }
    static const char* SN[4]={"literals","offsets","lengths","commands"};
    for(int i=0;i<NF;i++){ CK(cudaMemcpy(d_bad,d_in,a.size(),cudaMemcpyDeviceToDevice));
        size_t at=a.size()/2+rng()%(a.size()-a.size()/2); uint8_t b=a[at]^(uint8_t)(1+rng()%255); CK(cudaMemcpy(d_bad+at,&b,1,cudaMemcpyHostToDevice));
        int sx=0; while(sx<3 && at>=sb[sx+1]) sx++;
        long fk=-1; for(size_t j=0;j<HP.nv.size();j++) if(at>=HP.nv[j].in_off && at<HP.nv[j].in_off+HP.nv[j].csz){ fk=(long)j; break; }
        long pk=-1; for(size_t j=0;j<HP.rans.size();j++) if(at>=HP.rans[j].src && at<HP.rans[j].src+HP.rans[j].csz){ pk=(long)j; break; }
        printf("[+%.1f s] flip %d/%d at %zu (%s stream), 0x%02x -> 0x%02x: ",now_s(),i+1,NF,at,SN[sx],a[at],b);
        bool zbad=false;                                                   // libzstd rejects the flipped frame
        if(fk>=0){ const agp::Nv& J=HP.nv[fk]; std::vector<uint8_t> fr(a.begin()+J.in_off,a.begin()+J.in_off+J.csz); fr[at-J.in_off]=b;
            std::vector<uint8_t> o1(J.osz+64), o2(J.osz+64); const size_t z1=ZSTD_decompress(o1.data(),J.osz,fr.data(),fr.size()); ZSTD_decompress(o2.data(),J.osz,a.data()+J.in_off,J.csz);
            const std::string rp=std::string(RD)+"/"+base+".flip"+std::to_string(i+1)+".frame"+std::to_string(fk);
            put(rp+".orig.zst",a.data()+J.in_off,J.csz); put(rp+".flip.zst",fr.data(),fr.size()); zbad=ZSTD_isError(z1) || z1!=J.osz;
            printf("zstd frame %ld of %zu (%s), byte %llu of %llu, decoded size %llu; libzstd on the CPU: %s; frame header check %d; saved %s.{orig,flip}.zst\n",
                fk,HP.nv.size(),(size_t)fk<HP.NT?"token":"literal",(unsigned long long)(at-J.in_off),(unsigned long long)J.csz,(unsigned long long)J.osz,
                ZSTD_isError(z1)?ZSTD_getErrorName(z1):(z1==J.osz&&!memcmp(o1.data(),o2.data(),z1)?"ok, same bytes":"ok, other bytes"),agp::zstd_frame_check(fr.data(),fr.size(),J.osz),rp.c_str());
            // nvCOMP alone on this frame (batch of 1) - only when libzstd decodes it: nvCOMP 5.3 does not finish on some
            // frames libzstd rejects (verify/repro/README.md)
            if(!zbad){
            uint8_t *fz,*fo; void* ft; const void** fcp; void** fop; size_t *fcs,*fos,*fact; nvcompStatus_t* fst; size_t tmp=0;
            nvcompBatchedZstdDecompressGetTempSizeAsync(1,J.osz,nvcompBatchedZstdDecompressDefaultOpts,&tmp,J.osz);
            CK(cudaMalloc(&fz,J.csz+256)); CK(cudaMalloc(&fo,J.osz+256)); CK(cudaMalloc(&ft,tmp+256)); CK(cudaMalloc(&fcp,8)); CK(cudaMalloc(&fop,8));
            CK(cudaMalloc(&fcs,8)); CK(cudaMalloc(&fos,8)); CK(cudaMalloc(&fact,8)); CK(cudaMalloc(&fst,sizeof(nvcompStatus_t)));
            const void* hcp=fz; void* hop=fo; const size_t hcs=J.csz, hos=J.osz;
            CK(cudaMemcpy(fz,fr.data(),J.csz,cudaMemcpyHostToDevice)); CK(cudaMemcpy(fcp,&hcp,8,cudaMemcpyHostToDevice)); CK(cudaMemcpy(fop,&hop,8,cudaMemcpyHostToDevice));
            CK(cudaMemcpy(fcs,&hcs,8,cudaMemcpyHostToDevice)); CK(cudaMemcpy(fos,&hos,8,cudaMemcpyHostToDevice));
            const double tn=now_s(); std::string w="flip "+std::to_string(i+1)+": nvCOMP alone on zstd frame "+std::to_string(fk);
            if(nvcompBatchedZstdDecompressAsync(fcp,fcs,fos,fact,1,ft,tmp,fop,nvcompBatchedZstdDecompressDefaultOpts,fst,s)==nvcompSuccess){
                wait(s,w.c_str()); nvcompStatus_t hs; size_t act=0; CK(cudaMemcpy(&hs,fst,sizeof hs,cudaMemcpyDeviceToHost)); CK(cudaMemcpy(&act,fact,8,cudaMemcpyDeviceToHost));
                printf("[+%.1f s]   %s: finished in %.3f s, status %d, %zu B\n",now_s(),w.c_str(),now_s()-tn,(int)hs,act); }
            else printf("[+%.1f s]   %s: launch refused\n",now_s(),w.c_str());
            cudaFree(fz); cudaFree(fo); cudaFree(ft); cudaFree(fcp); cudaFree(fop); cudaFree(fcs); cudaFree(fos); cudaFree(fact); cudaFree(fst); } }
        else if(pk>=0) printf("rANS piece %ld of %zu (class %u)\n",pk,HP.rans.size(),HP.rans[pk].pad);
        else printf("not in a zstd frame or rANS piece (stored bytes, chunk table or padding)\n");
        // the flipped archive as a caller gets it: its own plan. With zstd frames the plan validates them
        // (ACEAPEX_GPU_VALIDATE_ZSTD, the mode for untrusted archives) and the device decodes with that plan; a
        // refused plan is caught before any launch. The open profile has no frames: the intact plan, as before.
        aceapex_gpu_plan* fp=plan; bool fref=false;
        { std::vector<uint8_t> hb=a; hb[at]=b; const unsigned pf=HP.nv.empty()?0:ACEAPEX_GPU_VALIDATE_ZSTD; const double tq=now_s();
          aceapex_gpu_plan* q=aceapex_gpu_plan_create(hb.data(),hb.size(),pf);
          printf("[+%.1f s]   plan of the flipped archive%s: %s (%.3f s)\n",now_s(),pf?" with ACEAPEX_GPU_VALIDATE_ZSTD":"",
                 q?"built (the damage is past the host checks)":"REFUSED (fail-closed before any launch)",now_s()-tq);
          if(q && pf && (aceapex_gpu_temp_bytes(q)>tb || aceapex_gpu_output_bytes(q)!=n)){ printf("[+%.1f s]   its layout needs other buffers: counted as refused\n",now_s()); aceapex_gpu_plan_destroy(q); q=nullptr; }
          if(!q){ refused++; fref=true; } else if(pf) fp=q; else aceapex_gpu_plan_destroy(q); }
        if(fk>=0 && zbad && !fref) printf("[+%.1f s]   NOTE: libzstd rejects the frame but the plan was built\n",now_s());
        for(int v=0;v<2;v++){ const double tf0=now_s(); g_flip=i+1; g_flipv=v?" + XXH3":"";
            if(fref){ caught[v]++; printf("[+%.1f s] flip %d/%d at %zu (%s stream)%s: refused by the plan, caught (no launch)\n",now_s(),i+1,NF,at,SN[sx],v?" + XXH3":""); continue; }
            aceapex_gpu_debug_phase_hook(on_phase);
            r=aceapex_gpu_decompress_async(fp,d_bad,d_out,d_temp,d_st,v?ACEAPEX_GPU_VERIFY_XXH3:0,s); wait(s,v?"flip decode + XXH3":"flip decode"); CK(cudaGetLastError());
            aceapex_gpu_debug_phase_hook(nullptr);
            CK(cudaMemcpy(&st,d_st,4,cudaMemcpyDeviceToHost)); CK(cudaMemcpy(out.data(),d_out,n,cudaMemcpyDeviceToHost));
            const char* k; if(r||st){ caught[v]++; k="caught"; } else if(memcmp(out.data(),orig.data(),n)){ silent[v]++; k="SILENT"; } else { same[v]++; k="harmless"; }
            printf("[+%.1f s] flip %d/%d at %zu (%s stream)%s: return %d, status %d, %s (%.2f s)\n",now_s(),i+1,NF,at,SN[sx],v?" + XXH3":"",r,st,k,now_s()-tf0); }
        if(fp!=plan) aceapex_gpu_plan_destroy(fp); }
    printf("[api] flips refused by the plan of the flipped archive%s: %d of %d\n",HP.nv.empty()?"":" (ACEAPEX_GPU_VALIDATE_ZSTD)",refused,NF);
    printf("[api] %d byte flips in the stream half of the archive: without the hash check %d caught by status, %d decoded wrong with status 0, %d harmless; with ACEAPEX_GPU_VERIFY_XXH3 %d caught, %d silent, %d harmless; no CUDA error\n",
        NF,caught[0],silent[0],same[0],caught[1],silent[1],same[1]);
    printf("APIROW\t%s\t%zu\t%.3f\t%s\t%d\t%d\t%.3f\t%d\t%d\t%d\t%.1f\t%.3f\t%s\t%d\t%d\t%d\t%.1f\t%d\t%.3f\t%s\n",argv[1],a.size(),api_ms,full_ok?"bit-perfect":"MISMATCH",rok,NR,r16,caught[0],silent[0],same[0],plan_ms,
        ver_ms,vfull_ok?"bit-perfect":"MISMATCH",caught[1],silent[1],same[1],val_ms,refused,tile_ms,tile_ms<0?"-":(tile_ok?"bit-perfect":"MISMATCH"));
    aceapex_gpu_plan_destroy(plan);
    return (full_ok && vfull_ok && rok==NR && silent[1]==0 && (tile_ms<0 || tile_ok)) ? 0 : 5;
}
