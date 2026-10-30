#!/usr/bin/env bash
# gpu_run.sh - the GPU run on any machine with an NVIDIA GPU: RunPod (H100: the main card; persistent storage in
# /workspace), Colab (Blackwell / A100 / L4 / T4: Drive at /content/drive) or any CUDA host. One line:
#   git clone -q https://github.com/yasha1971-coder/aceapex && ONE=1 bash aceapex/scripts/gpu_run.sh
# On RunPod the pod stops itself at the end (logs copied to /workspace/aceapex_logs first); KEEP=1 to keep it.
# Modes: QUICK (default): builds, emulators, fixtures, chr1.open + t2t.open (tool with [open variants] / [tile variants],
#   library); FULL=1 or ONE=1: every profile, dense, flips, repro and the corpus ladder below in one go (one stay on the
#   card), SUMMARY at the end; ONE also runs the card tests T-H1..T-H4 (HTESTS=1 runs them in QUICK). HPRC=1 adds the
#   pangenome rung (downloads ~8.5 GB of .fa.gz, 30 GB unpacked; AGC and MBGC on the same inputs).
# Time and downloads (estimates; the store keeps corpora and archives for the next run):
#   QUICK            ~20-30 min (Colab G4 / Blackwell)  ~1.1 GB: chr1 0.07 + T2T 0.9 GB gz + nvCOMP wheel 0.1
#   QUICK HTESTS=1   +10 min                            + samtools / tabix packages (~30 MB)
#   ONE              ~70-100 min (H100)                 ~2.1 GB: + GRCh38 0.95 GB gz; disk ~45 GB in the work dir
#   ONE HPRC=1       +45-70 min                         + ~8.5 GB: 10 assemblies (.fa.gz; 30 GB unpacked), AGC / MBGC
# Corpus ladder (md5 / sha256 pinned; from the store, else downloaded and kept in the store):
#   chr1    hg38 chr1 (the canon of the papers)          UCSC chromosomes/chr1.fa.gz            md5 9465e0f0... (fa)
#   t2t     T2T-CHM13 v2.0                                NCBI GCA_009914755.4 genomic.fna.gz    md5 cd1e52ce... (fa)
#   grch38  GRCh38 whole (UCSC hg38.fa.gz)                ONE=1                                  md5 b2aee9f8... (fa)
#   hprc    HPRC year-1 assemblies, first HPRC_N (10)      HPRC=1, index of HPP_Year1_Assemblies  sha256 per file (gz)
# Store: Colab with Drive: MyDrive/aceapex_corpus (corpora) + cache/ (archives); RunPod: /workspace/aceapex_store;
# elsewhere $HOME/aceapex_store (STORE=... overrides). Speed gate: results/baseline_<card>.tsv for this card only.
# Log: results/<platform>-<date>-<card>-gpu.log; the SUMMARY block and the log are also copied to <store>/logs.
set -uo pipefail
cd "$(dirname "$0")/.."; D=$(date -u +%Y-%m-%d); mkdir -p results
GPU=$(nvidia-smi --query-gpu=name --format=csv,noheader | head -n 1)
TAG=$(echo "$GPU" | tr 'A-Z' 'a-z' | sed 's/^nvidia //; s/^tesla //; s/[^a-z0-9]\+/-/g; s/^-//; s/-$//')
# platform and storage
if [ -d /content ] && python3 -c "import google.colab" >/dev/null 2>&1; then PLATFORM=colab; W=${WORK:-/content/work}
  if [ -d /content/drive/MyDrive ]; then STORE=${STORE:-/content/drive/MyDrive/aceapex_corpus}; else STORE=${STORE:-/content/aceapex_store}; fi
elif [ -d /workspace ]; then PLATFORM=runpod; W=${WORK:-/workspace/work}; STORE=${STORE:-/workspace/aceapex_store}
else PLATFORM=host; W=${WORK:-$HOME/aceapex_work}; STORE=${STORE:-$HOME/aceapex_store}; fi
mkdir -p $W $STORE/cache $STORE/logs
# files a run leaves in the work dir (on RunPod /workspace persists between pods): the 834d5d1 H100 run found ~30 GB of
# unpacked HPRC FASTA from the run before and ran out of space - a 3 GB round-trip file and one HPRC assembly were cut
rm -f $W/rt.bin $W/ex.out $W/d.fa $W/h.fa $W/agc_one.fa $W/hprc_*.fa $W/hprc.agc $W/hprc.mbgc; rm -rf $W/mbgc_out
DRV=$STORE; HAVE_DRIVE=1                                   # (names kept from colab_gpu_open.sh: the store plays the Drive's part)
# The run's log is written on the local disk and copied to results/ and the store at the end (and by the exit trap):
# on RunPod /workspace is a network volume, and the many appending writers of one log there left a hole of NUL bytes
# (H100 02.10: the AGC/MBGC lines and the first table lines lost; grep then saw a binary file, "bit-perfect 3 of 0").
LF=results/$PLATFORM-$D-$TAG-gpu.log; LOC=$(mktemp -d /tmp/aceapex_run.XXXXXX); : > $LOC/.start; L=$LOC/$(basename $LF)
ONE=${ONE:-0}; FULL=${FULL:-0}; [ "$ONE" = 1 ] && FULL=1
[ "$FULL" = 1 ] && MODE=FULL || MODE=QUICK; [ "$ONE" = 1 ] && MODE=ONE
TO=${STEP_TIMEOUT:-600}; ETO=${ENC_TIMEOUT:-1800}; exec 3>&1
# RunPod: on any exit (done, failed, interrupted) the logs go to /workspace (persistent volume) and the pod stops, so an
# unattended run does not keep billing; KEEP=1 leaves the pod running. Needs RUNPOD_POD_ID (set by RunPod) and runpodctl.
# On any exit: the log goes to results/ and the store, the SUMMARY (or why there is none) and the verdict to the console;
# on RunPod the logs also go to /workspace/aceapex_logs, everything is synced to the volume (a stop right after a copy
# lost it on 02.10) and checked, then the pod stops; KEEP=1 keeps it.
finish(){ local rc=$?; trap - EXIT
  if [ -s $LOC/summary.txt ]; then cat $LOC/summary.txt >&3
  else local last; last=$(tail -n 15 $L 2>/dev/null)
    { echo "== SUMMARY == $(date -u +%FT%TZ) - none: the run stopped early (exit $rc); last lines of the log:"; echo "$last"; echo "verdict FAILED (stopped early, exit $rc)"; } | tee -a $L >&3; fi
  mkdir -p results $STORE/logs; cp -f $L $LF; cp -f $L $STORE/logs/$(basename $LF .log)-$(git rev-parse --short HEAD).log 2>/dev/null
  if [ -n "${RUNPOD_POD_ID:-}" ]; then mkdir -p /workspace/aceapex_logs
    cp -f $L /workspace/aceapex_logs/; find results -name "*.log" -newer $LOC/.start -exec cp -f -t /workspace/aceapex_logs/ {} + 2>/dev/null; [ -s $LOC/summary.txt ] && cp -f $LOC/summary.txt /workspace/aceapex_logs/summary-$D.txt
    for f in $W/*.log; do [ -f "$f" ] && cp -f "$f" /workspace/aceapex_logs/; done
    sync; sleep 5; sync
    if cmp -s $L /workspace/aceapex_logs/$(basename $LF); then echo "logs: /workspace/aceapex_logs ($(ls /workspace/aceapex_logs | wc -l) files, $(du -sh /workspace/aceapex_logs | cut -f1)), checked" >&3
    else echo "logs: the copy in /workspace/aceapex_logs does not match - pod kept running" >&3; KEEP=1; fi
    if [ "${KEEP:-0}" != 1 ]; then echo "auto-stop: stopping pod $RUNPOD_POD_ID (KEEP=1 keeps it)" >&3
      if command -v runpodctl >/dev/null; then runpodctl stop pod "$RUNPOD_POD_ID"; else echo "auto-stop: runpodctl not found - stop the pod by hand" >&3; fi; fi
  else sync; fi
  return $rc; }
trap finish EXIT
# to <seconds> <step name> <command...>: the command under timeout; on expiry the TIMEOUT line goes to the log and
# to the cell (fd 3, not into the caller's pipe), exit status 124
to(){ local lim=$1 nm=$2; shift 2; timeout -k 20 $lim "$@"; local rc=$?
  if [ $rc = 124 ] || [ $rc = 137 ]; then echo "TIMEOUT $lim s: $nm" | tee -a $L >&3; return 124; fi; return $rc; }
RAM_GB=$(awk '/MemTotal/{printf "%d", $2/1048576}' /proc/meminfo)
case "$GPU" in *H100*) BASEF=results/baseline_h100.tsv;; *"RTX PRO 6000"*) BASEF=results/baseline_blackwell.tsv;; *) BASEF=results/baseline_$TAG.tsv;; esac
{ echo "MODE $MODE ($( [ $MODE = QUICK ] && echo 'open profile on chr1 and t2t, tool + library; FULL=1 / ONE=1 for the rest' || echo 'every profile, dense, flips, repro, corpus ladder'$( [ "${HPRC:-0}" = 1 ] && echo ' + HPRC' )))"
  echo "platform $PLATFORM | store $STORE | work $W"
  echo "== provenance $D"
  echo "gpu $GPU | cc $(nvidia-smi --query-gpu=compute_cap --format=csv,noheader | head -n 1) | driver $(nvidia-smi --query-gpu=driver_version --format=csv,noheader | head -n 1)"
  echo "clocks max sm/mem $(nvidia-smi --query-gpu=clocks.max.sm,clocks.max.mem --format=csv,noheader | head -n 1) | memory $(nvidia-smi --query-gpu=memory.total --format=csv,noheader | head -n 1) | power limit $(nvidia-smi --query-gpu=power.limit --format=csv,noheader | head -n 1)"
  echo "host $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 | sed 's/^ //'), $(nproc) threads, RAM $RAM_GB GB, disk free $(df -BG --output=avail $W | tail -1 | tr -d ' ')"
  nvcc --version | tail -1; g++ --version | head -1; echo "commit $(git rev-parse --short HEAD)"; } 2>&1 | tee $L
[ -f /usr/include/zstd.h ] || { apt-get -qq update && apt-get -qq install -y libzstd-dev >/dev/null; }
grep -h '#define ZSTD_VERSION_\(MAJOR\|MINOR\|RELEASE\)' /usr/include/zstd.h | awk '{print $3}' | paste -sd. - | sed 's/^/libzstd /' | tee -a $L
pip -q install nvidia-nvcomp-cu12 2>&1 | grep -v 'requires\|incompatible' | tail -1
NV=$(dirname "$(dirname "$(find / -name libnvcomp.so.5 -path '*libnvcomp/lib64*' 2>/dev/null | head -n 1)")")
[ -f "$NV/include/nvcomp/zstd.h" ] || { echo "nvCOMP not found" | tee -a $L; exit 1; }
pip show nvidia-nvcomp-cu12 2>/dev/null | grep -i '^version' | sed 's/^/nvcomp /' | tee -a $L
export LD_LIBRARY_PATH=$NV/lib64:${LD_LIBRARY_PATH:-}
SM=$(nvidia-smi --query-gpu=compute_cap --format=csv,noheader | head -n 1 | tr -d '.')
to $TO "make" make -s 2>&1 | grep -i error; [ -x ./aceapex ] || { echo "no CLI" | tee -a $L; exit 1; }
NVL="-I$NV/include -L$NV/lib64 -l:libnvcomp.so.5"
ARCH="-arch=sm_$SM"; nvcc -arch=sm_$SM -E -x cu /dev/null >/dev/null 2>&1 || ARCH="-gencode arch=compute_90,code=compute_90"
# GPU builds in parallel (each nvcc is one host thread); bld <name> <command...>: exit status in $W/bld.<name>
bld(){ local n=$1; shift; ( to $TO "build $n" "$@" > $W/bld.$n.err 2>&1; echo $? > $W/bld.$n ) & }
bld aceapex_gpu nvcc -O3 $ARCH $NVL -o $W/aceapex_gpu aceapex_gpu.cu
bld gpu_api_test nvcc -std=c++17 -O3 $ARCH -DACEAPEX_ENV_TUNING -Isrc -DACEAPEX_GPU_NVCOMP $NVL -o $W/gpu_api_test scripts/gpu_api_test.cu src/aceapex_gpu_lib.cu src/aceapex_gpu_abi.cpp -lzstd
bld gpu_decode nvcc -std=c++17 -O3 $ARCH -Isrc -o $W/gpu_decode examples/gpu_decode.cu src/aceapex_gpu_lib.cu src/aceapex_gpu_abi.cpp
bld gpu_stream nvcc -std=c++17 -O3 $ARCH -Isrc -DACEAPEX_GPU_NVCOMP $NVL -o $W/gpu_stream scripts/gpu_stream.cu src/aceapex_gpu_lib.cu src/aceapex_gpu_abi.cpp -lzstd
HT=0; { [ $MODE = ONE ] || [ "${HTESTS:-0}" = 1 ]; } && HT=1      # the four card tests T-H1..T-H4 (scripts/gpu_h100_tests.cu)
[ $HT = 1 ] && bld gpu_h100_tests nvcc -std=c++17 -O3 $ARCH -Isrc -DACEAPEX_GPU_NVCOMP $NVL -o $W/gpu_h100_tests scripts/gpu_h100_tests.cu src/aceapex_gpu_lib.cu src/aceapex_gpu_abi.cpp src/aceapex_api.cpp -lzstd -lpthread
bld gpu_lib sh -c "make -s gpu-lib NVCOMP=$NV GPU_ARCH='$ARCH' && nvcc -std=c++17 -O3 $ARCH -Isrc -o $W/gpu_decode_so examples/gpu_decode.cu -L. -laceapex_gpu -Xlinker -rpath=$(pwd) -Xlinker -rpath-link=$NV/lib64"
if [ $MODE = FULL ]; then
  # the same tool with AX_VEC=0 (byte stores in the match kernel and k_unpack, as before 01.10): chr1 before/after
  bld aceapex_gpu_v0 nvcc -O3 $ARCH -DAX_VEC=0 $NVL -o $W/aceapex_gpu_v0 aceapex_gpu.cu
  bld nvcomp_frame_repro nvcc -std=c++17 -O3 $ARCH $NVL -o $W/nvcomp_frame_repro scripts/nvcomp_frame_repro.cu -lzstd
fi
# CPU judges of the device steps and of the library's plan, meanwhile
for e in rans_warp_emu open_warp_emu; do
  g++ -std=c++17 -O2 -Isrc -o $W/$e scripts/$e.cpp && to $TO "$e" $W/$e verify/fixtures/conf/*.aet | tee -a $L; done
g++ -std=c++17 -O2 -DACEAPEX_ENV_TUNING -Isrc -o $W/gpu_plan_emu scripts/gpu_plan_emu.cpp src/aceapex_api.cpp -lzstd -lpthread && to $TO "gpu_plan_emu" $W/gpu_plan_emu | tee -a $L
wait
okb(){ [ "$(cat $W/bld.$1 2>/dev/null)" = 0 ]; }
okb aceapex_gpu && echo "built aceapex_gpu $ARCH" | tee -a $L || { echo "BUILD FAILED aceapex_gpu" | tee -a $L; tail -20 $W/bld.aceapex_gpu.err; exit 1; }
okb gpu_api_test && okb gpu_decode && echo "built gpu_api_test (nvCOMP) and examples/gpu_decode (no nvCOMP), $ARCH" | tee -a $L \
  || { echo "BUILD FAILED gpu library" | tee -a $L; tail -20 $W/bld.gpu_api_test.err $W/bld.gpu_decode.err; exit 1; }
okb gpu_lib && echo "shared library libaceapex_gpu.so.1: $(readelf -d libaceapex_gpu.so.1 | grep -o 'soname: \[[^]]*\]'), API version $(grep -m1 -o 'VERSION_MAJOR [0-9]*' src/aceapex_gpu.h | cut -d' ' -f2).$(grep -m1 -o 'VERSION_MINOR [0-9]*' src/aceapex_gpu.h | cut -d' ' -f2); examples/gpu_decode linked against it" | tee -a $L \
  || { echo "shared library build FAILED" | tee -a $L; tail -5 $W/bld.gpu_lib.err; }
if [ $MODE = FULL ]; then
  okb aceapex_gpu_v0 || echo "build aceapex_gpu AX_VEC=0 failed (no before/after line)" | tee -a $L
  # saved corrupt zstd frames (verify/repro/README.md): nvCOMP alone on each (batch of 1) against libzstd - a frame nvCOMP
  # hangs on stops only this step (watchdog 60 s: line NVCOMP HANG, informational - nvCOMP 5.3.0.16 hangs on the
  # flipped frame; the library's guard is ACEAPEX_GPU_VALIDATE_ZSTD)
  if okb nvcomp_frame_repro; then
    echo "== nvcomp_frame_repro" | tee -a $L
    AX_WATCHDOG=60 to $TO "nvcomp_frame_repro" $W/nvcomp_frame_repro verify/repro/t2t_frame150180.orig.zst 8192 verify/repro/t2t_frame150180.flip.zst 8192 2>&1 | tee -a $L
  else echo "build nvcomp_frame_repro failed" | tee -a $L; fi
fi

# the open conformance fixtures on the GPU (inputs regenerated from their seeds); 5 hashes each:
# warm-up, sequential, old pipeline, stream pipeline on cleared buffers, stream pipeline timed
python3 scripts/make_fixtures.py --regen >/dev/null
FX="dna_open_2MiB dna_open_mixed_300K dna_open_4097B dna_openlit_300K text_open_200K"; FXBAD=""
for f in $FX; do
  r=$(to $TO "fixture $f" $W/aceapex_gpu verify/fixtures/conf/$f.aet /tmp/conf_inputs/$f auto 3 1 --pipeline=3 2>&1); rc=$?
  echo "fixture $f on the GPU: exit $rc, $(echo "$r" | grep -c 'MATCHES OK') MATCHES OK, $(echo "$r" | grep -c 'DIFFERS X') DIFFERS" | tee -a $L
  [ $rc = 0 ] || { FXBAD="$FXBAD $f"; echo "$r" | grep -v '^probe' | tail -3 | sed 's/^/  /' | tee -a $L; }
done
CS=$(command -v compute-sanitizer || ls /usr/local/cuda/bin/compute-sanitizer 2>/dev/null)
for f in $FXBAD; do [ -n "$CS" ] || break
  echo "== compute-sanitizer $f" | tee -a $L
  to $TO "compute-sanitizer $f" $CS --tool memcheck --show-backtrace device $W/aceapex_gpu verify/fixtures/conf/$f.aet /tmp/conf_inputs/$f 16 1 1 2>&1 \
    | grep -v '^probe\|^\[' | grep -m 12 -i 'error\|kernel\|at 0x\|by thread\|in \|====' | tee -a $L
  break   # the first failing fixture is enough to name the kernel
done

# corpus: from the store (plain or .gz), else downloaded (to a file first: a cut stream gave a truncated corpus, L4
# 29.09), the download checked (md5 or sha256 of the .gz when given), unpacked, the corpus checked (md5 when pinned;
# an unpinned corpus prints its md5 for the pin), kept in the store
get_corpus(){ # name fa_md5 url [gz_sum] -> $W/name or return 1
  local n=$1 m=$2 u=$3 gs=${4:-} c=$W/$1
  if [ ! -s $c ]; then
    if [ -s $STORE/$n ]; then cp $STORE/$n $c; echo "$n: from the store" | tee -a $L
    elif [ -s $STORE/$n.gz ]; then gunzip -c $STORE/$n.gz > $c; echo "$n: from the store ($n.gz)" | tee -a $L
    elif [ -n "$u" ]; then
      for try in 1 2 3; do curl -fsSL --retry 3 -o $c.gz "$u" && gzip -t $c.gz 2>/dev/null && break; rm -f $c.gz; done
      [ -s $c.gz ] || { echo "$n: download failed ($u)" | tee -a $L; return 1; }
      if [ -n "$gs" ]; then local got; [ ${#gs} = 64 ] && got=$(sha256sum $c.gz | cut -d' ' -f1) || got=$(md5sum $c.gz | cut -d' ' -f1)
        [ "$got" = "$gs" ] || { echo "$n: CHECKSUM MISMATCH of the download ($got != $gs)" | tee -a $L; rm -f $c.gz; return 1; }; fi
      gunzip -c $c.gz > $c; cp $c.gz $STORE/$n.gz 2>/dev/null && echo "$n: downloaded, kept in the store" | tee -a $L; rm -f $c.gz
    else echo "$n: not in the store ($STORE/$n or $n.gz) and no source - skipped" | tee -a $L; return 1; fi
  fi
  if [ -z "$m" ]; then echo "$n: md5 $(md5sum $c | cut -d' ' -f1) (not pinned yet)" | tee -a $L; return 0; fi
  echo "$m  $c" | md5sum -c - >/dev/null 2>&1 && echo "$n: md5 $m OK" | tee -a $L || { echo "$n: MD5 MISMATCH" | tee -a $L; rm -f $c; return 1; }
}
# archive bytes; zstd frames depend on the libzstd version (t2t: 1.5.5 on Colab, 1.4.8 on ace-core),
# the open archive has none; chr1 at 16 KiB blocks came out the same under both
ZV=$(grep -h '#define ZSTD_VERSION_\(MAJOR\|MINOR\|RELEASE\)' /usr/include/zstd.h | awk '{print $3}' | paste -sd. -)
# pins since ADR-020 (l1 encoder is the DNA default): zstd/rans archives depend on libzstd (1.4.8 and 1.5.5,
# the latter from the official zstd-1.5.5 tarball on ace-core: its chain sizes equal the Colab 1.5.5 archives),
# open/chain do not. chain = the open profile with the matcher before ADR-020.
pinned(){ case $1.$2 in
  chr1.zstd) case $ZV in 1.4.8) echo 60449427;; 1.5.5) echo 60442704;; esac;;
  chr1.rans) case $ZV in 1.4.8) echo 60431000;; 1.5.5) echo 60424124;; esac;;
  chr1.open) echo 63091473;; chr1.chain) echo 68178789;;   # open: 16 KiB token chunks since 02.10 (63083287 / 67975888 before)
  t2t.zstd) case $ZV in 1.4.8) echo 822680738;; 1.5.5) echo 822393156;; esac;;
  t2t.rans) case $ZV in 1.4.8) echo 822818235;; 1.5.5) echo 822531418;; esac;;
  t2t.open) echo 853265321;; t2t.chain) echo 890229151;; esac; }   # (853264869 / 887641942 before)
T=$(nproc); CORP=""
get_corpus chr1.fa 9465e0f0df6e2c6eb39729c39cee5465 https://hgdownload.soe.ucsc.edu/goldenPath/hg38/chromosomes/chr1.fa.gz && CORP="chr1"
get_corpus t2t.fa cd1e52ce400c027ed0b7ab4b9d613f5a https://ftp.ncbi.nlm.nih.gov/genomes/all/GCA/009/914/755/GCA_009914755.4_T2T-CHM13v2.0/GCA_009914755.4_T2T-CHM13v2.0_genomic.fna.gz 9280657210e4161147cbe13b022225b9 && CORP="$CORP t2t"
[ $MODE = ONE ] && get_corpus grch38.fa b2aee9f885accc00531e59c4736bee63 https://hgdownload.soe.ucsc.edu/goldenPath/hg38/bigZips/hg38.fa.gz 1c9dcaddfa41027f17cd8f7a82c7293b && CORP="$CORP grch38"
for X in $CORP; do
  C=$W/$X.fa
  PROFS="open"; [ $MODE = FULL ] && PROFS="zstd rans open chain"
  for P in $PROFS; do
    case $P in zstd) E="FSE_CHUNK=4096";; rans) E="AX_TOK=rans";; open) E="AX_PROFILE=open";; chain) E="AX_PROFILE=open AX_ENC=chain";; esac
    A=$W/$X.$P.aet; PIN=$(pinned $X $P); SRC=encoded
    # an archive left in $W by an earlier run is reused only when it has the pinned size (30.09: a stale
    # chain-encoded open archive from the run before ADR-020 was reported as encoded by this build)
    if [ -s $A ]; then if [ -n "$PIN" ] && [ "$(stat -c%s $A)" = "$PIN" ]; then SRC="reused from $W (== pin)"; else rm -f $A; fi; fi
    if [ ! -s $A ] && [ $HAVE_DRIVE = 1 ] && [ -n "$PIN" ] && [ "$(stat -c%s $DRV/cache/$X.$P.aet 2>/dev/null)" = "$PIN" ]; then cp $DRV/cache/$X.$P.aet $A; SRC="from Drive cache"; fi
    if [ ! -s $A ]; then
      if [ $X = t2t ] && [ $RAM_GB -lt 20 ]; then echo "$X.$P: RAM $RAM_GB GB < 20 GB for the encoder (11.2 GB RSS) and no cached archive - skipped; run once on a host with more RAM (A100/G4) to fill $DRV/cache" | tee -a $L; continue; fi
      # encoder RAM and time on the big rungs: /usr/bin/time when present (peak RSS), else wall time only
      t_enc0=$(date +%s.%N); TM=""; [ -x /usr/bin/time ] && TM="/usr/bin/time -f %M -o $W/enc.rss"
      to $ETO "encode $X.$P" $TM env -i PATH=$PATH ACEAPEX_BS=16384 LIT_CHUNK=65536 $E ./aceapex c --in $C --out $A --threads $T >/dev/null 2>&1 || rm -f $A
      echo "encode $X.$P: $(awk -v a=$t_enc0 -v b=$(date +%s.%N) 'BEGIN{printf "%.1f", b-a}') s on $T threads$( [ -s $W/enc.rss ] && echo ", peak RSS $(awk '{printf "%.1f", $1/1048576}' $W/enc.rss) GB")$( [ -s $A ] && echo ", $(stat -c%s $A) B")" | tee -a $L; rm -f $W/enc.rss
      [ $HAVE_DRIVE = 1 ] && [ -n "$PIN" ] && [ "$(stat -c%s $A)" = "$PIN" ] && mkdir -p $DRV/cache && cp $A $DRV/cache/ && SRC="encoded, cached on Drive"
    fi
    [ -s $A ] || { echo "$X.$P: no archive - skipped" | tee -a $L; continue; }
    # streamed to cmp: no output file in the work dir (a 3 GB file there failed on a full volume, H100 834d5d1)
    S=$(stat -c%s $A); if [ -z "$PIN" ]; then PS="no pin for libzstd $ZV"; elif [ "$S" = "$PIN" ]; then PS="== pinned"; else PS="!= pinned $PIN"; fi
    if to $TO "CPU decode $X.$P" sh -c "env -i PATH=\"$PATH\" ./aceapex d --in $A -c 2>/dev/null | cmp -s - $C"; then
      echo "$X.$P ($E): archive $S B ($PS), $SRC, CPU round-trip bit-perfect" | tee -a $L
    else [ $P = chain ] && echo "$X.$P: estimate row, CPU round-trip failed" | tee -a $L \
      || echo "$X.$P: CPU ROUND-TRIP FAILED (work dir free: $(df -h --output=avail $W | tail -n 1 | tr -d ' '))" | tee -a $L; fi
  done
  # dense-open (measurement, not the format): the literal stream of the open archive coded by
  # components/rans1_v4.c (order-1 rANS, 4 lines, checkpoints every 4096), decoded on the GPU by k_r1
  if [ $MODE = FULL ] && [ -s $W/$X.open.aet ]; then
    [ -x $W/rans1_v4 ] || gcc -O3 -march=native -o $W/rans1_v4 components/rans1_v4.c -lm
    [ -x $W/rans1_seg ] || gcc -O3 -march=native -o $W/rans1_seg components/rans1_seg.c -lm
    [ -x $W/dense2_lane_emu ] || g++ -O2 -o $W/dense2_lane_emu scripts/dense2_lane_emu.cpp
    R0=$(pwd); ( cd $W && to $TO "stream dump $X.open" env -i PATH=$PATH ACEAPEX_DUMP=1 $R0/aceapex d --in $X.open.aet --out rt.bin >/dev/null 2>&1 ); rm -f $W/rt.bin
    python3 - $W/streams.bin $W/$X.lit <<'PY'
import struct,sys
s=open(sys.argv[1],'rb').read(); nb=struct.unpack_from('<I',s,24)[0]; H=68; b=H+64*(nb-1)
lo=struct.unpack_from('<Q',s,b)[0]; ls=struct.unpack_from('<Q',s,b+32)[0]
open(sys.argv[2],'wb').write(s[H+64*nb:H+64*nb+lo+ls])
PY
    rm -f $W/streams.bin; to $TO "rans1_v4 $X" $W/rans1_v4 c $W/$X.lit $W/$X.r1 >/dev/null 2>&1
    # dense-open v2: 32 segments per chunk, context reset per segment, slot tables (components/rans1_seg.c, k_r2)
    to $TO "rans1_seg $X" $W/rans1_seg c $W/$X.lit $W/$X.r2 >/dev/null 2>&1; echo "$X.dense2 lane emulator: $(to $TO "dense2_lane_emu $X" $W/dense2_lane_emu $W/$X.r2 $W/$X.lit | tr '\n' ' ')" | tee -a $L; rm -f $W/$X.lit
    DB=$(python3 -c "import struct,os;a=open('$W/$X.open.aet','rb').read(80);print(os.path.getsize('$W/$X.open.aet')-struct.unpack_from('<Q',a,36)[0]+os.path.getsize('$W/$X.r1'))")
    echo "$DB" > $W/$X.dense.bytes
    DB2=$(python3 -c "import struct,os;a=open('$W/$X.open.aet','rb').read(80);print(os.path.getsize('$W/$X.open.aet')-struct.unpack_from('<Q',a,36)[0]+os.path.getsize('$W/$X.r2'))")
    echo "$DB2" > $W/$X.dense2.bytes
    echo "$X.dense2 (open + rans1_seg order-1 literals, 32 segments/chunk, estimate = open - literal stream + AR2L file): $DB2 B, AR2L $(stat -c%s $W/$X.r2) B" | tee -a $L
    echo "$X.dense (open + rans1_v4 order-1 literals, estimate = open - literal stream + AR1L file): $DB B, AR1L $(stat -c%s $W/$X.r1) B" | tee -a $L
  fi
  for P in $PROFS; do [ -s $W/$X.$P.aet ] || continue
    DL=""; [ $P = open ] && [ -s $W/$X.r1 ] && DL="--dense-lit=$W/$X.r1"; [ $P = open ] && [ -s $W/$X.r2 ] && DL="$DL --dense2-lit=$W/$X.r2"
    R=${REPS:-3}
    if [ $P = zstd ] || [ $P = open ]; then                   # the C ABI on the same archive: full, ranges, byte flips
      echo "== gpu_api_test $X.$P" | tee -a $L
      NRG=200; NFL=20; [ $X = t2t ] && { NRG=60; NFL=6; }; [ $MODE = QUICK ] && NFL=0
      AX_WATCHDOG=$((TO/2)) AX_REPRO=verify/repro/run to $TO "gpu_api_test $X.$P" $W/gpu_api_test $W/$X.$P.aet $C ${REPS:-3} $NRG $NFL 2>&1 | tee -a $L; echo "exitapi ${PIPESTATUS[0]} $X.$P" | tee -a $L
      # the flipped frames (original and flipped) kept on Drive for a repro
      [ $HAVE_DRIVE = 1 ] && ls verify/repro/run/*.zst >/dev/null 2>&1 && mkdir -p $DRV/repro && cp verify/repro/run/*.zst $DRV/repro/ && echo "flipped frames copied to $DRV/repro ($(ls verify/repro/run/*.zst | wc -l) files)" | tee -a $L
      if [ $P = open ]; then to $TO "gpu_decode $X.$P" $W/gpu_decode $W/$X.$P.aet $LOC/ex.out >/dev/null 2>&1 && cmp -s $LOC/ex.out $C && echo "example gpu_decode $X.$P (no nvCOMP): bit-perfect" | tee -a $L \
        || echo "example gpu_decode $X.$P: FAILED" | tee -a $L; rm -f $LOC/ex.out
        if [ -x $W/gpu_decode_so ]; then to $TO "gpu_decode (shared) $X.$P" $W/gpu_decode_so $W/$X.$P.aet $LOC/ex.out >/dev/null 2>&1 && cmp -s $LOC/ex.out $C \
          && echo "example gpu_decode $X.$P via libaceapex_gpu.so.1 (nvCOMP inside): bit-perfect" | tee -a $L || echo "example gpu_decode $X.$P via libaceapex_gpu.so.1: FAILED" | tee -a $L; rm -f $LOC/ex.out; fi; fi
    fi
    echo "== aceapex_gpu $X.$P" | tee -a $L
    to $TO "aceapex_gpu $X.$P" $W/aceapex_gpu $W/$X.$P.aet $C auto $R 4 --pipeline=${PIPE:-auto} $DL 2>&1 | tee -a $L; echo "exit ${PIPESTATUS[0]} $X.$P" | tee -a $L
    # before/after of the 16-byte stores (AX_VEC): the AX_VEC=0 tool on the same archive right after; its ROW stays out of the log
    if [ $X = chr1 ] && [ -x $W/aceapex_gpu_v0 ] && { [ $P = zstd ] || [ $P = open ]; }; then
      to $TO "aceapex_gpu AX_VEC=0 $X.$P" $W/aceapex_gpu_v0 $W/$X.$P.aet $C auto $R 4 --pipeline=${PIPE:-auto} > $W/v0.txt 2>&1
      awk -F'\t' -v a="$W/$X.$P.aet" -v x="$X.$P" 'FNR==NR{ if($1=="ROW" && $2==a){u0=$8; m0=$9; d0=$10; k0=$13} next } $1=="ROW" && $2==a{u1=$8; m1=$9; d1=$10; k1=$13}
        END{ if(d0!="" && d1!="") printf "vec %s (16-byte stores, AX_VEC 0 -> 1): match %.3f -> %.3f ms (%+.1f %%), unpack %.3f -> %.3f ms, on-device %.3f -> %.3f ms (%+.1f %%), %s / %s\n", x, m0, m1, (m0>0?100*(m1/m0-1):0), u0, u1, d0, d1, (d0>0?100*(d1/d0-1):0), k0, k1;
             else printf "vec %s: no ROW from one of the tools\n", x }' $W/v0.txt $L | tee -a $L
    fi
  done
done

# output larger than the card (ONE): block-range plans (aceapex_gpu_plan_create_blocks), batches of K blocks sized from
# the free device memory, two in flight (decode of i+1 while i goes to the host), XXH3 of the whole output on the host
# against the header, every batch compared with the original. Once with the automatic K, once with slots capped at
# 256 MB (many batches) on t2t - the mode a corpus larger than the card takes.
if [ $MODE = ONE ] && [ -x $W/gpu_stream ]; then
  for X in grch38 t2t; do [ -s $W/$X.open.aet ] || continue
    echo "== gpu_stream $X.open (batches from the free memory)" | tee -a $L
    to $TO "gpu_stream $X.open" $W/gpu_stream $W/$X.open.aet $W/$X.fa 2>&1 | tee -a $L; break; done
  if [ -s $W/t2t.open.aet ]; then echo "== gpu_stream t2t.open (slots capped at 256 MB: many batches)" | tee -a $L
    AX_STREAM_MAX_MB=256 to $TO "gpu_stream t2t.open capped" $W/gpu_stream $W/t2t.open.aet $W/t2t.fa 2>&1 | sed 's/^STREAMROW/STREAMROW_CAP/' | tee -a $L; fi
fi
# the four card tests (ONE, or HTESTS=1 in any mode; scripts/gpu_h100_tests.cu), on t2t.open:
#   T-H1 saturation: K decodes of t2t at once (25-50 GB of output on 80 GB), peak GB/s; each copy XXH3-checked
#   T-H2 PCIe pipeline: pinned archive -> pieces of 4 / 8 MB (tables, token streams first) -> block batches decode as
#        their literal chunks arrive, outputs in a ring of 4 slots;
#        against H2D alone and the resident decode; every batch compared with the original
#   T-H3 random access: 10 000 regions of 5 000 bases by coordinate - GPU (resident, pooled, direct and CUDA Graph),
#        CPU region call, samtools faidx on bgzip (process per region and -r); p50 / p99, every result == FASTA
#        (tabix indexes tab-separated files, not FASTA: bgzip + samtools faidx is the htslib way for sequence)
#   T-H4 stress: 10 000 corrupt copies of a 16 MB chr1 slice (open profile): 0 hangs, 0 silent with XXH3
if [ $HT = 1 ] && [ -x $W/gpu_h100_tests ] && [ -s $W/t2t.open.aet ]; then
  echo "== T-H1 saturation" | tee -a $L; to $TO "T-H1" $W/gpu_h100_tests sat $W/t2t.open.aet 2>&1 | tee -a $L
  echo "== T-H2 PCIe pipeline" | tee -a $L; to $TO "T-H2" $W/gpu_h100_tests pcie $W/t2t.open.aet $W/t2t.fa 2>&1 | tee -a $L
  # samtools / bgzip for T-H3's comparison rows (not part of the verdict: without them the rows read "-")
  command -v samtools >/dev/null && command -v bgzip >/dev/null || { apt-get -qq update >/dev/null 2>&1; DEBIAN_FRONTEND=noninteractive apt-get -qq install -y samtools tabix >/dev/null 2>&1 \
    || echo "samtools: apt install failed - T-H3 without the samtools rows" | tee -a $L; }
  SAMV=$(samtools --version 2>/dev/null | head -n 1); echo "== T-H3 random access (${SAMV:-no samtools})" | tee -a $L
  if [ -n "$SAMV" ] && [ ! -s $W/t2t.fa.bgz.gzi ]; then bgzip -@ $T -c $W/t2t.fa > $W/t2t.fa.bgz && samtools faidx $W/t2t.fa.bgz && echo "bgzip t2t: $(stat -c%s $W/t2t.fa.bgz) B" | tee -a $L; fi
  if [ -n "$SAMV" ]; then to $((TO*2)) "T-H3" $W/gpu_h100_tests ra $W/t2t.open.aet $W/t2t.fa 10000 5000 samtools $W/t2t.fa.bgz 2>&1 | tee -a $L
  else to $TO "T-H3" $W/gpu_h100_tests ra $W/t2t.open.aet $W/t2t.fa 10000 5000 2>&1 | tee -a $L; fi
  # the same regions on an archive with the token chunks of 64 KiB the open profile wrote before 02.10 (FSE_CHUNK=65536;
  # since 02.10 it writes 16 KiB: a region decodes 3 token chunks with one warp each, 2048 against 512 sequential steps)
  env -i PATH=$PATH ACEAPEX_BS=16384 LIT_CHUNK=65536 AX_PROFILE=open FSE_CHUNK=65536 ./aceapex c --in $W/t2t.fa --out $W/t2t.ra16.aet >/dev/null 2>&1 \
    && echo "== T-H3b random access, 64 KiB token chunks as before 02.10 ($(stat -c%s $W/t2t.ra16.aet) B)" | tee -a $L \
    && to $TO "T-H3b" $W/gpu_h100_tests ra $W/t2t.ra16.aet $W/t2t.fa 10000 5000 2>&1 | sed 's/^H3ROW/H3BROW/' | tee -a $L; rm -f $W/t2t.ra16.aet
  # T-H5 data loader: random windows (1 / 8 / 32 KiB x 256 / 4096 / 65536 per batch) from the resident t2t.open archive into
  # one device buffer (aceapex_gpu_decompress_windows_async, the selection on the device), against region calls on 16 CPU
  # threads + H2D; 1 % of the windows compared with the original
  echo "== T-H5 data loader" | tee -a $L; to $((TO*2)) "T-H5" $W/gpu_h100_tests dl $W/t2t.open.aet $W/t2t.fa $T 2>&1 | tee -a $L
  if [ -s $W/chr1.fa ]; then tail -c +100000001 $W/chr1.fa | head -c 16777216 > $W/stress.fa
    env -i PATH=$PATH ACEAPEX_BS=16384 LIT_CHUNK=65536 AX_PROFILE=open ./aceapex c --in $W/stress.fa --out $W/stress.open.aet >/dev/null 2>&1
    echo "== T-H4 stress ($(stat -c%s $W/stress.open.aet 2>/dev/null) B archive of a 16 MB chr1 slice)" | tee -a $L
    to $((TO*2)) "T-H4" $W/gpu_h100_tests stress $W/stress.open.aet $W/stress.fa 10000 2>&1 | tee -a $L; fi
fi
# HPRC rung (HPRC=1): the first HPRC_N (10) haplotype assemblies of the HPRC year-1 index (sha256 per file), each encoded
# in the open profile (peak RSS and time), decoded on the GPU through the library (full + 20 ranges), against AGC and MBGC
# on the same files (archive size, compression and decompression time, one assembly back)
if [ "${HPRC:-0}" = 1 ]; then
  HN=${HPRC_N:-10}; HD=$STORE/hprc; mkdir -p $HD; IDX=$HD/index.tsv
  echo "== HPRC: first $HN assemblies of the year-1 index" | tee -a $L
  [ -s $IDX ] || curl -fsSL -o $IDX https://raw.githubusercontent.com/human-pangenomics/HPP_Year1_Assemblies/main/assembly_index/Year1_assemblies_v2_genbank.index || echo "HPRC: index download failed" | tee -a $L
  awk -F'\t' 'NR>1{print $1".1\t"$2"\t"$6; print $1".2\t"$3"\t"$7}' $IDX 2>/dev/null | head -n $HN > $HD/list.tsv
  HLIST=""
  while IFS=$'\t' read -r nm url sha; do
    u=${url/s3:\/\/human-pangenomics/https:\/\/s3-us-west-2.amazonaws.com\/human-pangenomics}
    if [ ! -s $HD/$nm.fa.gz ]; then to $ETO "download $nm" curl -fsSL --retry 3 -o $HD/$nm.fa.gz.part "$u" && mv $HD/$nm.fa.gz.part $HD/$nm.fa.gz; fi
    [ -s $HD/$nm.fa.gz ] || { echo "HPRC $nm: download failed" | tee -a $L; continue; }
    [ "$(sha256sum $HD/$nm.fa.gz | cut -d' ' -f1)" = "$sha" ] || { echo "HPRC $nm: SHA256 MISMATCH" | tee -a $L; rm -f $HD/$nm.fa.gz; continue; }
    HLIST="$HLIST $nm"
  done < $HD/list.tsv
  echo "HPRC: $(echo $HLIST | wc -w) of $HN assemblies (sha256 OK)" | tee -a $L
  HRAW=0; HAET=0; HENC=0; HLIB=0; HMISS=""
  for nm in $HLIST; do
    gunzip -c $HD/$nm.fa.gz > $W/h.fa || { echo "HPRC $nm: unpack failed (work dir free: $(df -h --output=avail $W | tail -n 1 | tr -d ' ')) - skipped" | tee -a $L; HMISS="$HMISS $nm"; continue; }
    A=$HD/$nm.open.aet
    if [ ! -s $A ]; then t0=$(date +%s.%N); TM=""; [ -x /usr/bin/time ] && TM="/usr/bin/time -f %M -o $W/enc.rss"
      to $ETO "encode $nm" $TM env -i PATH=$PATH ACEAPEX_BS=16384 LIT_CHUNK=65536 AX_PROFILE=open ./aceapex c --in $W/h.fa --out $A --threads $T >/dev/null 2>&1 || rm -f $A
      echo "HPRC $nm encode: $(awk -v a=$t0 -v b=$(date +%s.%N) 'BEGIN{printf "%.1f", b-a}') s$( [ -s $W/enc.rss ] && echo ", peak RSS $(awk '{printf "%.1f", $1/1048576}' $W/enc.rss) GB")" | tee -a $L; rm -f $W/enc.rss; fi
    [ -s $A ] || continue
    HRAW=$((HRAW + $(stat -c%s $W/h.fa))); HAET=$((HAET + $(stat -c%s $A)))
    AX_WATCHDOG=$((TO/2)) to $TO "gpu_api_test $nm" $W/gpu_api_test $A $W/h.fa 3 20 0 2>&1 | grep '^APIROW\|DIFFERS\|TIMEOUT' | sed "s#^APIROW#APIROW_HPRC#" | tee -a $L
  done
  awk -F'\t' -v miss="$HMISS" -v hn="$(echo $HLIST | wc -w)" '$1=="APIROW_HPRC"{n++; ms+=$4; ok+=($5=="bit-perfect")} END{ if(n) printf "HPRC GPU library: %d of %d assemblies, %.3f ms total full decode, %d bit-perfect%s\n", n, hn, ms, ok, (miss==""?"":"; not decoded:" miss) }' $L | tee -a $L
  [ $HAET -gt 0 ] && echo "HPRC ACEAPEX open: $HRAW B -> $HAET B (ratio $(awk -v a=$HRAW -v b=$HAET 'BEGIN{printf "%.2f", a/b}')), one archive per assembly" | tee -a $L
  # AGC and MBGC: release binaries for Linux x64 (latest release asset), the same assemblies (plain FASTA)
  rel(){ curl -fsSL https://api.github.com/repos/$1/releases | grep -o '"browser_download_url": *"[^"]*'"$2"'[^"]*"' | head -n 1 | sed 's/.*"\(http[^"]*\)"/\1/'; }
  mkdir -p $W/tools; FAS=""
  for nm in $HLIST; do gunzip -c $HD/$nm.fa.gz > $W/hprc_$nm.fa && FAS="$FAS $W/hprc_$nm.fa" || { echo "HPRC $nm: unpack for AGC/MBGC failed" | tee -a $L; rm -f $W/hprc_$nm.fa; }; done
  if [ -n "$FAS" ]; then
    # AGC 3.2.4 and MBGC 2.1.6 pinned (the GitHub API answered nothing on the pod, 834d5d1): AGC's release binary by its
    # URL, else built from the tag; MBGC built from the tag (cmake)
    curl -fsSL -o $W/tools/agc.tgz https://github.com/refresh-bio/agc/releases/download/v3.2.4/agc-v3.2.4-x64_linux.tar.gz && tar -xzf $W/tools/agc.tgz -C $W/tools 2>/dev/null
    AGC=$(find $W/tools -type f -name agc -perm -u+x | head -n 1)
    if [ -z "$AGC" ] && git clone -q --depth 1 --branch v3.2.4 --recursive https://github.com/refresh-bio/agc.git $W/tools/agc-src 2>/dev/null; then
      ( cd $W/tools/agc-src && make -j$T >/dev/null 2>&1 ); AGC=$(find $W/tools/agc-src/bin -type f -name agc -perm -u+x 2>/dev/null | head -n 1); fi
    if [ -n "$AGC" ]; then t0=$(date +%s.%N); to $ETO "agc create" $AGC create -t $T -o $W/hprc.agc $FAS >/dev/null 2>&1
      t1=$(date +%s.%N); f1=$(basename $(echo $FAS | cut -d' ' -f2) .fa); to $TO "agc getset" $AGC getset $W/hprc.agc $f1 > $W/agc_one.fa 2>/dev/null; t2=$(date +%s.%N)
      [ -s $W/hprc.agc ] || echo "HPRC AGC: create failed" | tee -a $L
      [ -s $W/hprc.agc ] && echo "HPRC AGC $($AGC 2>&1 | head -n 1 | tr -d '\r'): $(stat -c%s $W/hprc.agc 2>/dev/null) B, create $(awk -v a=$t0 -v b=$t1 'BEGIN{printf "%.1f", b-a}') s ($T threads), one assembly back $(awk -v a=$t1 -v b=$t2 'BEGIN{printf "%.1f", b-a}') s" | tee -a $L
    else echo "HPRC AGC: no Linux x64 release binary found - skipped" | tee -a $L; fi
    MB=$(find $W/tools -type f -name mbgc -perm -u+x | head -n 1)
    if [ -z "$MB" ]; then command -v cmake >/dev/null || apt-get -qq install -y cmake >/dev/null 2>&1
      git clone -q --depth 1 --branch v2.1.6 --recursive https://github.com/kowallus/mbgc.git $W/tools/mbgc-src 2>/dev/null \
        && ( mkdir -p $W/tools/mbgc-src/build && cd $W/tools/mbgc-src/build && cmake .. -DCMAKE_BUILD_TYPE=Release >/dev/null 2>&1 && make -j$T mbgc >/dev/null 2>&1 )
      MB=$(find $W/tools/mbgc-src/build -maxdepth 1 -type f -name mbgc -perm -u+x 2>/dev/null | head -n 1); fi
    if [ -n "$MB" ]; then ls $FAS > $W/mbgc_list.txt; t0=$(date +%s.%N); to $ETO "mbgc compress" $MB c $W/mbgc_list.txt $W/hprc.mbgc >/dev/null 2>&1
      t1=$(date +%s.%N); mkdir -p $W/mbgc_out; to $ETO "mbgc decompress" $MB d $W/hprc.mbgc $W/mbgc_out >/dev/null 2>&1; t2=$(date +%s.%N)
      [ -s $W/hprc.mbgc ] || echo "HPRC MBGC: compress failed (command line: mbgc c <list> <archive>)" | tee -a $L
      [ -s $W/hprc.mbgc ] && echo "HPRC MBGC: $(stat -c%s $W/hprc.mbgc 2>/dev/null) B, compress $(awk -v a=$t0 -v b=$t1 'BEGIN{printf "%.1f", b-a}') s, decompress all $(awk -v a=$t1 -v b=$t2 'BEGIN{printf "%.1f", b-a}') s" | tee -a $L
    else echo "HPRC MBGC: no Linux release binary found - skipped" | tee -a $L; fi
    # ACEAPEX on the CPU of this host: every archive back, and one assembly
    t0=$(date +%s.%N); for nm in $HLIST; do [ -s $HD/$nm.open.aet ] && ./aceapex d --in $HD/$nm.open.aet --out $W/d.fa >/dev/null 2>&1; done; t1=$(date +%s.%N)
    echo "HPRC ACEAPEX CPU decompress all: $(awk -v a=$t0 -v b=$t1 'BEGIN{printf "%.1f", b-a}') s ($T threads, one archive per assembly = one assembly back in 1/$HN of it)" | tee -a $L
    rm -f $W/d.fa $W/agc_one.fa $FAS $W/hprc.agc $W/hprc.mbgc; rm -rf $W/mbgc_out          # 30 GB of FASTA do not stay for the next run
  fi
  rm -f $W/h.fa
fi

# tables per corpus; TSV lines (gpu, corpus, row) for the table across GPUs
for X in $CORP; do
  grep -q "^ROW	$W/$X\." $L || continue
  echo; echo "$X on $GPU, ms, median of ${REPS:-3}; pipeline = chosen path (${PIPE:-auto}: stream pipeline when H2D >= on-device/2 and >= 2 batches of 64 MB, else sequential); batches 0 = sequential" | tee -a $L.t1
  { printf 'archive\tbytes\ttokens\tliterals\ttok\tlit\tunpack\tmatch\ton-device\t+H2D\tpipeline\tbatches\tGB/s\tcheck\tH2D-pageable\tH2D-pinned\n'
    grep "^ROW	$W/$X\." $L | awk -F'\t' -v OFS='\t' '{print $2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$24,$27,$12,$13,$25" GB/s",$26" GB/s"}' | sed "s#$W/##"
    DBY=$(cat $W/$X.dense.bytes 2>/dev/null); grep "^ROW	$W/$X\.open" $L | awk -F'\t' -v OFS='\t' -v db="$DBY" -v x="$X" '$30!="-1"{print x".dense (est.)",db,"rANS","order-1",$6,$28,"0",$9,$29,"-","-","-","-",($30=="0"?"bit-perfect":"DIFFERS"),"-","-"}'
    DB2=$(cat $W/$X.dense2.bytes 2>/dev/null); grep "^ROW	$W/$X\.open" $L | awk -F'\t' -v OFS='\t' -v db="$DB2" -v x="$X" '$35!="" && $35!="-1"{l=1e9; for(k=31;k<=33;k++) if($k>=0 && $k<l) l=$k; print x".dense2 (est.)",db,"rANS","o1 32seg",$6,sprintf("%.3f",l)" (byte "$31" / nib "$32" / cmp "$33")","0",$9,$34,"-","-","-","-",($35=="0"?"bit-perfect":"DIFFERS"),"-","-"}'; } | column -t -s $'\t' | tee -a $L.t1
  echo "parts, ms: lit = zstd frames + pieces by class; unpack = zstd-pack kernels + open kernels" | tee -a $L.t1
  { printf 'archive\tlit.zstd\tseq\tcse\tgap\tval\tplain\tun.zstdpack\tbases\tcase\texceptions\n'
    grep "^ROW	$W/$X\." $L | awk -F'\t' -v OFS='\t' '{print $2,$14,$15,$16,$17,$18,$19,$20,$21,$22,$23}' | sed "s#$W/##"; } | column -t -s $'\t' | tee -a $L.t1
done
[ -f $L.t1 ] && cat $L.t1 >> $L; rm -f $L.t1
grep '^ROW' $L | sed "s#$W/##" | awk -F'\t' -v OFS='\t' -v g="$TAG" '{$1="TSV\t" g; print}' | tee -a $L
# open (l1 encoder since ADR-020) against chain (the same profile with the matcher before ADR-020), per corpus
for X in $CORP; do
  awk -F'\t' -v x="$X" -v w="$W/" '$1=="ROW" && $2==w x".chain.aet"{o=$10; ob=$3} $1=="ROW" && $2==w x".open.aet"{l=$10; lb=$3; lk=$13}
    END{ if(o!="" && l!="") printf "%s.open (l1) against %s.chain: on-device %.3f vs %.3f ms (%+.1f %%), bytes %d vs %d (%+.2f %%), %s\n", x, x, l, o, 100*(l/o-1), lb, ob, 100*(lb/ob-1), lk }' $L | tee -a $L
done
# C ABI against the measurement tool (same archive, same run): on-device ms of the tool (ROW $10) and of the library
for X in $CORP; do for P in zstd open; do
  awk -F'\t' -v a="$W/$X.$P.aet" -v x="$X.$P" '$1=="ROW" && $2==a{t=$10} $1=="APIROW" && $2==a{api=$4; ok=$5; r=$6"/"$7; r16=$8; c=$9; si=$10; h=$11; vm=$13; vok=$14; vc=$15; vs=$16; vh=$17; pv=$18; pr=$19}
    END{ if(api!="") printf "api %s: library %.3f ms vs tool %.3f ms on-device (%+.1f %%), %s, ranges %s == original, 16 KiB window %.3f ms, flips caught/silent/harmless %s/%s/%s; with XXH3 check %.3f ms (%+.3f ms), %s, flips %s/%s/%s; plan with VALIDATE_ZSTD %s ms, flips refused by the plan %s\n", x, api, t, (t>0?100*(api/t-1):0), ok, r, r16, c, si, h, vm, vm-api, vok, vc, vs, vh, pv, pr }' $L | tee -a $L
done; done
# verdict: each archive run is valid on its own (the GPU output is hashed against the original);
# the run as a whole needs both emulators, the 5 fixtures, the chr1 open row, and no failure line.
# Estimate and reference rows (dense, dense2, chain) do not enter it: they have their own check columns / rule line.
RUN=$(grep -a '^exit [0-9]* ' $L | grep -vc '\.chain$'); OK=$(grep -a '^exit 0 ' $L | grep -vc '\.chain$'); N=$(awk -F'\t' '$1=="ROW" && $13=="bit-perfect" && $2 !~ /\.chain\.aet$/' $L | wc -l)
E=$(grep -ac $'^head_\(rans\|open\)_warp_emu\tpass' $L); F=$(grep -ac '^fixture .*: exit 0, [1-9][0-9]* MATCHES OK, 0 DIFFERS$' $L)   # any number of passes, none differing
echo "archives on the GPU: bit-perfect $N of $RUN (exit 0: $OK); emulators $E/2, fixtures $F/5" | tee -a $L
grep '^exit [1-9]' $L | sed 's/^/  FAILED: /; s/\.chain$/.chain (reference row, not in the verdict)/' | tee -a $L
AR=$(grep -ac '^exitapi ' $L); AOK=$(grep -ac '^exitapi 0 ' $L); PE=$(grep -ac $'^head_gpu_plan_emu\tpass' $L)
echo "C ABI: $AOK of $AR archives bit-perfect with every range; plan emulator $PE/1" | tee -a $L
grep '^exitapi [1-9]' $L | sed 's/^/  FAILED: /' | tee -a $L
NTO=$(grep -ac '^TIMEOUT ' $L); echo "steps over the time limit: $NTO" | tee -a $L
# speed gate against the baseline of this card (results/baseline_h100.tsv for H100, baseline_blackwell.tsv for the RTX PRO
# 6000 Blackwell; rows of the same GPU name only): library and tool on-device ms per archive, a
# row more than 5 % slower fails the verdict; faster rows are reported for a baseline update (a commit of its own)
awk -F'\t' -v w="$W/" -v g="$GPU" 'FNR==NR{ if($1==g && $7 !~ /re-measure/){ k=$2" "$3; base[k]=$4 } next }
  $1=="APIROW"{ k=$2; sub(w,"",k); sub(/\.aet$/,"",k); now[k" library"]=$4 }
  $1=="ROW"{ k=$2; sub(w,"",k); sub(/\.aet$/,"",k); now[k" tool"]=$10 }
  END{ for(k in base){ if(!(k in now) || now[k]+0<=0) continue; c=100*(now[k]/base[k]-1); v=(c>5?"SLOWER":(c<-5?"FASTER":"OK"))
         split(k,a," "); line[a[1]]=line[a[1]] sprintf("%s%s %.3f/%.3f ms (%+.1f %%) %s", (line[a[1]]==""?"":", "), a[2], now[k], base[k], c, v) }
       for(x in line) print "gate " x ": " line[x] }' $BASEF $L | sort > $LOC/gate.txt
[ -s $LOC/gate.txt ] || echo "gate: no baseline rows for $GPU ($BASEF) - this run can seed it" > $LOC/gate.txt
cat $LOC/gate.txt | tee -a $L
VERDICT=FAILED; WHY=""                                   # one evaluation: every failed condition named, with its count
FL=$(grep -a 'archive rejected\|ROUND-TRIP FAILED\|^example .*FAILED\|shared library build FAILED' $L | head -n 3 | tr '\n' '|')
NSLOW=$(grep -c SLOWER $LOC/gate.txt); NTH=$(grep -acE $'^H([1-5]|3B)ROW\t.*\tFAILED(\t|$)' $L)
[ "$N" = "$RUN" ] || WHY="$WHY; archives bit-perfect $N of $RUN"; [ "$E" = 2 ] || WHY="$WHY; emulators $E/2"; [ "$F" = 5 ] || WHY="$WHY; fixtures $F/5"
[ "$AOK" = "$AR" ] || WHY="$WHY; C ABI $AOK of $AR"; [ "$PE" = 1 ] || WHY="$WHY; plan emulator"; grep -aq "^ROW	$W/chr1.open" $L || WHY="$WHY; no chr1.open row"
[ -z "$FL" ] || WHY="$WHY; failure lines: ${FL%|}"; [ "$NTO" = 0 ] || WHY="$WHY; $NTO steps over the time limit"
[ "$NSLOW" = 0 ] || WHY="$WHY; speed gate: $NSLOW rows slower than $BASEF by > 5 %"; [ "$NTH" = 0 ] || WHY="$WHY; $NTH card test rows FAILED"
[ -z "$WHY" ] && VERDICT=PASSED
[ $VERDICT = PASSED ] && echo "RESULT: all passes bit-perfect on $GPU" | tee -a $L \
  || echo "!!! NOT PASSED on $GPU - valid figures only in bit-perfect rows" | tee -a $L
# == SUMMARY == (<= 15 lines): commit, mode, GPU; per archive library / tool on-device ms, stages, [open variants]; verdict.
# Also appended to <store>/logs/summary.txt with the whole log.
SIZES=""; for X in $CORP; do [ -s $W/$X.fa ] && SIZES="$SIZES $X=$(stat -c%s $W/$X.fa)"; done
{ echo "== SUMMARY == $(date -u +%FT%TZ)"
  echo "commit $(git rev-parse --short HEAD) | MODE $MODE | $GPU"
  awk -F'\t' -v w="$W/" -v sizes="$SIZES" '
    BEGIN{ n=split(sizes,a," "); for(i=1;i<=n;i++){ split(a[i],kv,"="); sz[kv[1]]=kv[2] } }
    $1=="ROW"{ k=$2; sub(w,"",k); sub(/\.aet$/,"",k); if(!(k in seen)){ seen[k]=1; ord[++m]=k } t[k]=$10; sq[k]=$15; un[k]=$8; ma[k]=$9; ck[k]=$13 }
    $1=="APIROW"{ k=$2; sub(w,"",k); sub(/\.aet$/,"",k); api[k]=$4 }
    $1=="TILEVAR"{ k=$2; sub(w,"",k); sub(/\.aet$/,"",k); tv[k]=sprintf("; tile %.3f->%.3f%s", $3, $4, ($5=="1"?"":" DIFFERS")) }
    $1=="OPENVAR"{ k=$2; sub(w,"",k); sub(/\.aet$/,"",k); var[k]=sprintf("; variants seq %.3f->%.3f, unpack %.3f->%.3f (EXC)->%.3f (SHB)%s", $3, $4, $5, $6, $7, ($8=="1"?"":" DIFFERS")) }
    END{ for(i=1;i<=m;i++){ k=ord[i]; x=k; sub(/\..*/,"",x); if(k !~ /\.(open|zstd)$/) continue
           lib = (k in api) ? sprintf("library %.3f ms (%.1f GB/s), ", api[k], (api[k]>0 && (x in sz)) ? sz[x]/api[k]/1e6 : 0) : ""
           printf "%s: %stool %.3f ms; seq %s, unpack %.3f, match %.3f ms; %s%s%s\n", k, lib, t[k], sq[k], un[k], ma[k], ck[k], var[k], tv[k] } }' $L
  grep -h '^HPRC GPU library\|^HPRC ACEAPEX open\|^HPRC AGC\|^HPRC MBGC' $L | head -n 4
  awk -F'\t' '$1=="STREAMROW"||$1=="STREAMROW_CAP"{ k=$2; sub(/.*\//,"",k); printf "stream %s%s: %s batches of %s blocks (slot %s MB), %.2f GB/s with D2H + host XXH3, hash %s, batches %s\n", k, ($1=="STREAMROW_CAP"?" (capped)":""), $5, $6, $7, $10, $11, $12 }' $L
  awk -F'\t' '$1=="H1ROW"{ printf "T-H1 saturation %s: peak %.1f GB/s with %s decodes at once (K: GB/s %s), XXH3 %s\n", $2, $5, $6, $7, $8 }
    $1=="H2ROW"{ printf "T-H2 PCIe: H2D %.1f GB/s; pieces: output GB/s / with D2H / output over bus %s; %s\n", $3, $4, $5 }
    $1=="H3ROW"||$1=="H3BROW"{ printf "T-H3%s %s regions x %s b, p50/p99 us: GPU %s (p50 launch/kernels/D2H/wait %s), GPU graph %s, CPU %s, samtools bgzip %s (one call: %s); %s\n", ($1=="H3BROW"?"b (64 KiB token chunks, before 02.10)":""), $3, $4, $5, $11, $6, $7, ($8=="0.0/0.0"?"-":$8), ($9<0?"-":sprintf("%.1f us/region",$9)), $10 }
    $1=="H5ROW"{ h5=h5 sprintf("%s%sK x %s: %.2fM w/s %.2f GB/s p50 %.2f ms host %.0f us (CPU+H2D %.0fk w/s, x%.1f)", (h5==""?"":"; "), $2/1024, $3, $4/1e6, $5, $6, $8, $11/1e3, $12); h5n++; if($13=="ok") h5ok++ }
    END{ if(h5n) printf "T-H5 data loader t2t.open (x = GPU / CPU 16 threads + H2D): %s; %d/%d %s\n", h5, h5ok, h5n, (h5ok==h5n?"ok, 1 % checked == original":"FAILED") }
    $1=="H4ROW"{ printf "T-H4 %s corrupt archives: refused %s, caught %s, harmless %s, silent %s (no hash) / %s (XXH3), hangs %s; %s\n", $3, $4, $5, $6, $7, $8, $9, $10 }' $L
  case "$GPU" in *H100*) echo "paper rows (README, H100 SXM, June 2026, not re-measured): FASTQ ERR194147 5 GB 168.9 GB/s ratio 3.31; 50 GB range decode 165.7 GB/s ratio 3.99; 5 GB genome full decode 29.71 ms";; esac
  cat $LOC/gate.txt
  echo "verdict $VERDICT${WHY:+ (${WHY#; })}"; } > $LOC/summary.txt
cat $LOC/summary.txt >> $L                               # the console gets it from the exit trap (finish)
# the summary appended and the whole log copied to Drive: the runtime may be released right after (runtime.unassign)
mkdir -p $STORE/logs && cat $LOC/summary.txt >> $STORE/logs/summary.txt     # the log itself: copied by finish
[ $VERDICT = PASSED ] || exit 1
