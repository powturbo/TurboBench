# research/ - measurements outside the format

Nothing here goes into ACEPX2 or the default archive bytes; tools build against the library sources in tuning builds.
Results: `results-2026-10-02.md`.

- `pangenome_refseg.cpp` - I3 (one archive of several HPRC assemblies, plain or with reference segments, depth <= 2)
  and I1 (31-mers counted from the tokens: literals and match edges only, against all windows; the sets compared).
- `run_i3.sh` - the I3 / I1 runs for 1, 2 and 4 assemblies.
- `run_pangenome_tools.sh` - AGC and MBGC (built from source) on the same assemblies.
- `selfheal.cpp` - I2 (a decoder repairs bit flips: the failing unit names the bytes, every bit there is tried, XXH3
  of the output is the oracle; `--sidecar`: block hashes as a sidecar; `SH_GPU=1`: probes through the GPU path).
