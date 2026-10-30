// gpu_zstd_batch: the entropy layer of an .aet archive decoded on the GPU with ONE call to
// nvCOMP's batched zstd decompressor (C API), timed properly: temp and output buffers allocated
// once, warm-up, N repeats on CUDA events, median. Output is reassembled and compared byte for
// byte with streams.bin (ACEAPEX_DUMP=1). The DNA unpack of the literal stream runs on the CPU
// and on the GPU (two kernels, timed), both compared with the CPU-decoded reference.
// Build: nvcc -O3 -arch=sm_XX -I$NVCOMP/include -L$NVCOMP/lib -lnvcomp -o gpu_zstd_batch gpu_zstd_batch.cu
// Usage: gpu_zstd_batch <archive.aet> <streams.bin> [repeats=7]
#include "src/ax_rans.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <cuda_runtime.h>
#include <nvcomp/zstd.h>

#define CK(x) do{cudaError_t e=(x); if(e!=cudaSuccess){fprintf(stderr,"CUDA %s @%d: %s\n",#x,__LINE__,cudaGetErrorString(e)); exit(2);} }while(0)
#define NV(x) do{nvcompStatus_t s=(x); if(s!=nvcompSuccess){fprintf(stderr,"nvcomp %s @%d: status %d\n",#x,__LINE__,(int)s); exit(3);} }while(0)


// --- GPU unpack of DNA literal chunks: 2-bit bases (MSB first) -> ACGT, case-mask bit -> |0x20,
// then exceptions: cumulative gaps give positions, val gives the byte (0 when no val stream).
struct DnaDesc { const uint8_t *seq,*cse,*gap,*val; uint8_t* dst; uint32_t raw, nexc; };
__global__ void k_unpack(const DnaDesc* d){
  const DnaDesc c=d[blockIdx.x]; uint32_t i0=(blockIdx.y*blockDim.x+threadIdx.x)*4; if(i0>=c.raw) return;
  uint8_t v=c.seq[i0>>2], m=c.cse[i0>>3]; uint32_t n=c.raw-i0; if(n>4) n=4;
  #pragma unroll
  for(uint32_t k=0;k<4;k++){ if(k<n){ uint8_t b="ACGT"[(v>>(6-2*k))&3]; if(m&(0x80>>((i0+k)&7))) b|=0x20; c.dst[i0+k]=b; } }
}
__global__ void k_exc(const DnaDesc* d){
  const DnaDesc c=d[blockIdx.x]; if(!c.nexc) return; __shared__ uint32_t s[256]; uint32_t carry=0;
  for(uint32_t base=0; base<c.nexc; base+=256){
    uint32_t e=base+threadIdx.x; uint32_t g = e<c.nexc ? ((const uint32_t*)c.gap)[e] : 0; s[threadIdx.x]=g; __syncthreads();
    for(uint32_t o=1;o<256;o<<=1){ uint32_t t = threadIdx.x>=o ? s[threadIdx.x-o] : 0; __syncthreads(); s[threadIdx.x]+=t; __syncthreads(); }
    uint32_t pos=carry+s[threadIdx.x]; if(e<c.nexc && pos<c.raw) c.dst[pos] = c.val ? c.val[e] : 0;
    carry+=s[255]; __syncthreads(); }
}

static std::vector<uint8_t> slurp(const char* p){ FILE* f=fopen(p,"rb"); if(!f){perror(p); exit(1);} fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET); std::vector<uint8_t> v(n); if(fread(v.data(),1,n,f)!=(size_t)n){fprintf(stderr,"short read %s\n",p); exit(1);} fclose(f); return v; }
static inline uint64_t rd64(const uint8_t* p){ uint64_t v; memcpy(&v,p,8); return v; }
static inline uint32_t rd32(const uint8_t* p){ uint32_t v; memcpy(&v,p,4); return v; }

// one zstd frame to decode: where its bytes are in the archive, how many bytes come out,
// and where the output goes (stream id, chunk id, sub-frame kind)
struct Job { const uint8_t* src; size_t csz; size_t osz; int stream; uint32_t chunk; int kind; };
enum { K_PLAIN=0, K_SEQ=1, K_CSE=2, K_GAP=3, K_VAL=4 };
struct DnaChunk { uint32_t chunk; size_t raw; uint32_t nexc; bool has_gap, has_val; };

int main(int argc, char** argv){
  if(argc<3){ fprintf(stderr,"usage: %s <archive.aet> <streams.bin> [repeats]\n",argv[0]); return 1; }
  int reps = argc>3 ? atoi(argv[3]) : 7;
  std::vector<uint8_t> a=slurp(argv[1]), s=slurp(argv[2]);
  // AetHeader 68 B packed: magic8 ver4 orig8 bs4 nb4 xx8 zl8 zo8 zn8 zc8
  uint64_t orig=rd64(&a[12]); uint32_t bs=rd32(&a[20]), nb=rd32(&a[24]);
  uint64_t zl=rd64(&a[36]), zo=rd64(&a[44]), zn=rd64(&a[52]), zc=rd64(&a[60]);
  const uint8_t* zs[4]; uint64_t zsz[4]={zl,zo,zn,zc}; size_t p=68+64ull*nb;
  for(int i=0;i<4;i++){ zs[i]=&a[p]; p+=zsz[i]; }
  // streams.bin: same header + BlockOffsets(4 offsets, 4 sizes) + decoded lit/off/len/cmd
  uint64_t tot[4]={0,0,0,0};
  for(uint32_t b=0;b<nb;b++) for(int i=0;i<4;i++) tot[i]+=rd64(&s[68+64ull*b+8*(4+i)]);
  const uint8_t* ref[4]; size_t q=68+64ull*nb; for(int i=0;i<4;i++){ ref[i]=&s[q]; q+=tot[i]; }
  printf("archive %s: orig=%llu block=%u nb=%u  z(lit,off,len,cmd)=%llu,%llu,%llu,%llu B\n", argv[1],
         (unsigned long long)orig, bs, nb, (unsigned long long)zl,(unsigned long long)zo,(unsigned long long)zn,(unsigned long long)zc);

  std::vector<Job> jobs; std::vector<uint8_t> out[4]; std::vector<DnaChunk> dna;
  // --- FSE-style streams off/len/cmd (stream ids 1..3): word0 = size | chunk/4096<<48; cs[nc]; frames
  for(int st=1; st<4; st++){
    const uint8_t* z=zs[st]; uint64_t w=rd64(z); uint64_t osz=w&((1ull<<48)-1); uint64_t ch=((w>>48)&0x7fff)*4096; if(!ch) ch=524288;
    uint64_t nc=(osz+ch-1)/ch; out[st].assign(osz,0); size_t pos=8+8*nc; int raws=0;
    for(uint64_t i=0;i<nc;i++){ uint64_t cs=rd64(z+8+8*i); size_t raw=std::min<uint64_t>(ch,osz-i*ch);
      if(cs>>63){ memcpy(&out[st][i*ch], z+pos, raw); pos+=raw; raws++; }
      else if((cs>>62)&1){ size_t csz=cs&((1ull<<48)-1);   // rANS token chunk (ADR-018): host decode
        if(axr_decode(z+pos,csz,&out[st][i*ch],raw)){ fprintf(stderr,"rANS chunk corrupt\n"); return 4; } pos+=csz; raws++; }
      else { size_t csz=cs&((1ull<<48)-1); jobs.push_back({z+pos,csz,raw,st,(uint32_t)i,K_PLAIN}); pos+=csz; } }
    printf("stream %d: size=%llu chunk=%llu frames=%llu raw=%d\n", st,(unsigned long long)osz,(unsigned long long)ch,(unsigned long long)(nc-raws),raws);
    if(osz!=tot[st]){ fprintf(stderr,"size mismatch stream %d: archive %llu vs streams.bin %llu\n",st,(unsigned long long)osz,(unsigned long long)tot[st]); return 4; }
  }
  // --- literal stream (id 0): word0 = size|bit62|bit61(chunked)|bit60(tagged); word1 = CH; zsz[NW]; chunks
  { const uint8_t* z=zs[0]; uint64_t h=rd64(z); uint64_t sz=h&~((1ull<<62)|(1ull<<61)|(1ull<<60)); bool tagged=h&(1ull<<60);
    uint64_t CH=rd64(z+8); uint64_t NW=(sz+CH-1)/CH; size_t pos=16+8*NW; out[0].assign(sz,0); int ndna=0;
    for(uint64_t t=0;t<NW;t++){ size_t raw=std::min<uint64_t>(CH,sz-t*CH); size_t csz=rd64(z+16+8*t); const uint8_t* c=z+pos; pos+=csz;
      if(tagged && c[0]==1){ uint32_t nexc=rd32(c+1),h1=rd32(c+5),h2=rd32(c+9),h3=rd32(c+13),h4=rd32(c+17); const uint8_t* f=c+21; ndna++;
        jobs.push_back({f,h1,(raw+3)/4,0,(uint32_t)t,K_SEQ}); f+=h1;
        jobs.push_back({f,h2,(raw+7)/8,0,(uint32_t)t,K_CSE}); f+=h2;
        if(h3) jobs.push_back({f,h3,(size_t)nexc*4,0,(uint32_t)t,K_GAP}); f+=h3;
        if(h4) jobs.push_back({f,h4,(size_t)nexc,0,(uint32_t)t,K_VAL});
        dna.push_back({(uint32_t)t,raw,nexc,h3!=0,h4!=0}); }
      else jobs.push_back({tagged?c+1:c, tagged?csz-1:csz, raw, 0,(uint32_t)t,K_PLAIN}); }
    printf("stream 0 (lit): size=%llu CH=%llu chunks=%llu dna=%d tagged=%d\n",(unsigned long long)sz,(unsigned long long)CH,(unsigned long long)NW,ndna,(int)tagged);
    if(sz!=tot[0]){ fprintf(stderr,"lit size mismatch %llu vs %llu\n",(unsigned long long)sz,(unsigned long long)tot[0]); return 4; } }

  // --- pack all frames into one device buffer; outputs into one device buffer; pointer tables
  size_t N=jobs.size(), cbytes=0, obytes=0, maxo=0; std::vector<size_t> coff(N), ooff(N);
  for(size_t i=0;i<N;i++){ coff[i]=cbytes; cbytes+=jobs[i].csz; ooff[i]=obytes; obytes+=(jobs[i].osz+63)&~size_t(63); maxo=std::max(maxo,jobs[i].osz); }
  printf("batch: %zu frames, %.1f MB compressed -> %.1f MB, max frame out %zu B\n", N, cbytes/1e6, obytes/1e6, maxo);
  uint8_t *hc; CK(cudaHostAlloc(&hc,cbytes,cudaHostAllocDefault)); for(size_t i=0;i<N;i++) memcpy(hc+coff[i],jobs[i].src,jobs[i].csz);
  uint8_t *dc,*dout; CK(cudaMalloc(&dc,cbytes)); CK(cudaMalloc(&dout,obytes));
  cudaEvent_t e0,e1; CK(cudaEventCreate(&e0)); CK(cudaEventCreate(&e1));
  CK(cudaEventRecord(e0)); CK(cudaMemcpyAsync(dc,hc,cbytes,cudaMemcpyHostToDevice)); CK(cudaEventRecord(e1)); CK(cudaEventSynchronize(e1));
  float h2d=0; CK(cudaEventElapsedTime(&h2d,e0,e1)); printf("H2D compressed (pinned): %.3f ms = %.1f GB/s\n",h2d,cbytes/h2d/1e6);
  std::vector<const void*> hcp(N); std::vector<void*> hop(N); std::vector<size_t> hcs(N), hos(N);
  for(size_t i=0;i<N;i++){ hcp[i]=dc+coff[i]; hop[i]=dout+ooff[i]; hcs[i]=jobs[i].csz; hos[i]=jobs[i].osz; }
  const void** dcp; void** dop; size_t *dcs,*dos,*dact; nvcompStatus_t* dst;
  CK(cudaMalloc(&dcp,N*sizeof(void*))); CK(cudaMalloc(&dop,N*sizeof(void*))); CK(cudaMalloc(&dcs,N*8)); CK(cudaMalloc(&dos,N*8)); CK(cudaMalloc(&dact,N*8)); CK(cudaMalloc(&dst,N*sizeof(nvcompStatus_t)));
  CK(cudaMemcpy(dcp,hcp.data(),N*sizeof(void*),cudaMemcpyHostToDevice)); CK(cudaMemcpy(dop,hop.data(),N*sizeof(void*),cudaMemcpyHostToDevice));
  CK(cudaMemcpy(dcs,hcs.data(),N*8,cudaMemcpyHostToDevice)); CK(cudaMemcpy(dos,hos.data(),N*8,cudaMemcpyHostToDevice));
  size_t temp=0; nvcompBatchedZstdDecompressOpts_t opts=nvcompBatchedZstdDecompressDefaultOpts;
  NV(nvcompBatchedZstdDecompressGetTempSizeAsync(N,maxo,opts,&temp,obytes)); void* dtemp; CK(cudaMalloc(&dtemp,temp)); printf("temp %.1f MB\n",temp/1e6);
  cudaStream_t str; CK(cudaStreamCreate(&str));
  auto run=[&](){ NV(nvcompBatchedZstdDecompressAsync(dcp,dcs,dos,dact,N,dtemp,temp,dop,opts,dst,str)); };
  run(); CK(cudaStreamSynchronize(str));                       // warm-up
  std::vector<float> ms(reps);
  for(int r=0;r<reps;r++){ CK(cudaEventRecord(e0,str)); run(); CK(cudaEventRecord(e1,str)); CK(cudaEventSynchronize(e1)); CK(cudaEventElapsedTime(&ms[r],e0,e1)); }
  std::sort(ms.begin(),ms.end()); float med=ms[reps/2];
  printf("[timed] nvCOMP batched zstd, %zu frames, %d runs: median %.3f ms (min %.3f max %.3f) -> %.1f GB/s of frame output, %.1f GB/s of compressed input\n",
         N,reps,med,ms[0],ms[reps-1],obytes/med/1e6,cbytes/med/1e6);
  std::vector<nvcompStatus_t> hst(N); std::vector<size_t> hact(N);
  CK(cudaMemcpy(hst.data(),dst,N*sizeof(nvcompStatus_t),cudaMemcpyDeviceToHost)); CK(cudaMemcpy(hact.data(),dact,N*8,cudaMemcpyDeviceToHost));
  size_t bad=0; for(size_t i=0;i<N;i++) if(hst[i]!=nvcompSuccess||hact[i]!=jobs[i].osz) bad++;
  printf("frame status: %zu bad of %zu\n",bad,N);
  std::vector<uint8_t> ho(obytes); CK(cudaMemcpy(ho.data(),dout,obytes,cudaMemcpyDeviceToHost));
  // --- reassemble: plain frames straight into place; DNA chunks unpacked on the CPU (timed)
  std::vector<const uint8_t*> seq(dna.size(),nullptr),cse(dna.size(),nullptr),gap(dna.size(),nullptr),val(dna.size(),nullptr);
  uint64_t CH=rd64(zs[0]+8); size_t di=0; std::vector<size_t> dmap; // job -> dna index by chunk
  { std::vector<int64_t> byChunk; for(auto& d:dna){ if(d.chunk>=byChunk.size()) byChunk.resize(d.chunk+1,-1); byChunk[d.chunk]=di++; }
    for(size_t i=0;i<N;i++){ const Job& j=jobs[i]; const uint8_t* o=ho.data()+ooff[i];
      if(j.kind==K_PLAIN){ size_t at = j.stream==0 ? (size_t)j.chunk*CH : 0; if(j.stream) { /* offset by chunk size of that stream */ uint64_t w=rd64(zs[j.stream]); uint64_t ch=((w>>48)&0x7fff)*4096; if(!ch) ch=524288; at=(size_t)j.chunk*ch; }
        memcpy(&out[j.stream][at],o,j.osz); }
      else { size_t k=byChunk[j.chunk]; (j.kind==K_SEQ?seq:j.kind==K_CSE?cse:j.kind==K_GAP?gap:val)[k]=o; } } }
  auto t0=std::chrono::steady_clock::now();
  for(size_t k=0;k<dna.size();k++){ const DnaChunk& d=dna[k]; uint8_t* o=&out[0][(size_t)d.chunk*CH];
    for(size_t i=0;i<d.raw;i++){ uint8_t v=seq[k][i>>2]; o[i]="ACGT"[(v>>(6-2*(i&3)))&3]; }
    for(size_t i=0;i<d.raw;i++) if(cse[k][i>>3]&(0x80>>(i&7))) o[i]|=0x20;
    if(d.nexc){ int64_t pos=0; for(uint32_t e=0;e<d.nexc;e++){ pos+=d.has_gap?rd32(gap[k]+4*e):0; if(pos<(int64_t)d.raw) o[pos]=d.has_val?val[k][e]:0; } } }
  double unpack_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t0).count();
  printf("CPU DNA unpack of %zu chunks: %.1f ms (GPU kernel is the next step)\n",dna.size(),unpack_ms);
  const char* nm[4]={"lit","off","len","cmd"}; int ok=0;
  // --- GPU unpack: device literal buffer, plain chunks copied D2D, DNA chunks by two kernels (timed)
  { uint64_t sz=out[0].size(); uint8_t* dlit; CK(cudaMalloc(&dlit,sz)); std::vector<DnaDesc> hd(dna.size());
    std::vector<size_t> dnaJob(dna.size()*4,(size_t)-1);   // (dna idx, kind-1) -> job index
    std::vector<int64_t> byChunk; for(size_t k=0;k<dna.size();k++){ if(dna[k].chunk>=byChunk.size()) byChunk.resize(dna[k].chunk+1,-1); byChunk[dna[k].chunk]=k; }
    for(size_t i=0;i<N;i++){ const Job& j=jobs[i]; if(j.stream!=0) continue;
      if(j.kind==K_PLAIN) CK(cudaMemcpyAsync(dlit+(size_t)j.chunk*CH, dout+ooff[i], j.osz, cudaMemcpyDeviceToDevice, str));
      else dnaJob[byChunk[j.chunk]*4+(j.kind-1)]=i; }
    for(size_t k=0;k<dna.size();k++){ auto P=[&](int kind)->const uint8_t*{ size_t i=dnaJob[k*4+kind-1]; return i==(size_t)-1?nullptr:dout+ooff[i]; };
      hd[k]={P(K_SEQ),P(K_CSE),P(K_GAP),P(K_VAL), dlit+(size_t)dna[k].chunk*CH, (uint32_t)dna[k].raw, dna[k].nexc}; }
    DnaDesc* dd; CK(cudaMalloc(&dd,hd.size()*sizeof(DnaDesc))); CK(cudaMemcpyAsync(dd,hd.data(),hd.size()*sizeof(DnaDesc),cudaMemcpyHostToDevice,str));
    dim3 g1((unsigned)dna.size(), (unsigned)((CH/4+255)/256));
    auto runk=[&](){ k_unpack<<<g1,256,0,str>>>(dd); k_exc<<<(unsigned)dna.size(),256,0,str>>>(dd); };
    runk(); CK(cudaStreamSynchronize(str)); CK(cudaGetLastError());
    std::vector<float> um(reps);
    for(int r=0;r<reps;r++){ CK(cudaEventRecord(e0,str)); runk(); CK(cudaEventRecord(e1,str)); CK(cudaEventSynchronize(e1)); CK(cudaEventElapsedTime(&um[r],e0,e1)); }
    std::sort(um.begin(),um.end()); float umed=um[reps/2];
    std::vector<uint8_t> hl(sz); CK(cudaMemcpy(hl.data(),dlit,sz,cudaMemcpyDeviceToHost));
    bool eq = memcmp(hl.data(),ref[0],sz)==0;
    printf("[timed] GPU DNA unpack (2 kernels), %zu chunks, %d runs: median %.3f ms (min %.3f max %.3f) -> %.1f GB/s of literals; bit-perfect=%s\n",
           dna.size(),reps,umed,um[0],um[reps-1],sz/umed/1e6,eq?"true":"FALSE");
    printf("[pipeline] entropy layer on GPU = zstd %.3f + unpack %.3f = %.3f ms for %.1f MB of streams -> %.1f GB/s\n",
           med,umed,med+umed,(tot[0]+tot[1]+tot[2]+tot[3])/1e6,(tot[0]+tot[1]+tot[2]+tot[3])/(med+umed)/1e6);
    if(!eq) ok=-100; }

  for(int i=0;i<4;i++){ bool eq = out[i].size()==tot[i] && memcmp(out[i].data(),ref[i],tot[i])==0; ok+=eq; printf("%s: %llu B bit-perfect=%s\n",nm[i],(unsigned long long)tot[i],eq?"true":"FALSE"); }
  printf("%d/4 streams bit-perfect; entropy layer GPU time %.3f ms for %.1f MB of streams -> %.1f GB/s\n", ok, med, (tot[0]+tot[1]+tot[2]+tot[3])/1e6, (tot[0]+tot[1]+tot[2]+tot[3])/med/1e6);
  return ok==4?0:5;
}
