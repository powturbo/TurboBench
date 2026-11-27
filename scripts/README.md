# Benchmark Markdown views (optional post-processing)

TurboBench prints Markdown with `-p4` (one table, C Size order). This folder adds:

- **Linked column headers** → one page per sort (`micro.md`, `micro_ratio.md`, `micro_CMBs.md`, …)
- **PNG charts** under `charts/` (GitHub does not embed TurboBench’s SVG inline in `.md`)

The `turbobench` binary is unchanged; run these steps **after** `-p4`.

## Quick test (no benchmark run)

Uses a tiny fixture (no codec build required):

```bash
python3 -m pip install -r scripts/requirements.txt   # once
./scripts/test_bench_views.sh
```

## MICRO benchmark workflow

```bash
make
./turbobench blaris/bench/data/* -U -eMICRO -aMICRO   # or your MICRO corpus
./turbobench data@MICRO.tbb -p4                        # → data.md (raw table)
./scripts/enhance_bench_md.sh data.md --stem micro --out-dir turbobench_
```

Or in one step from the `.tbb` file:

```bash
./scripts/enhance_bench_md.sh data@MICRO.tbb --stem micro --out-dir turbobench_
```

Published files:

| File | Role |
|------|------|
| `data.md` | TurboBench output (`-p4`); safe to regenerate / gitignore locally |
| `turbobench_/micro.md` | Default view (C Size order) + charts |
| `turbobench_/micro_*.md` | Same data, sorted by column |
| `turbobench_/charts/*.png` | Scatter + decompression bar chart |

Re-running `enhance_bench_md.sh` is idempotent (old chart blocks are stripped before rebuild).

## Manual API

```bash
python3 scripts/bench_views.py RAW.md --stem micro --out-dir turbobench_/
```

- **`RAW.md`**: output of `./turbobench something.tbb -p4` (not the enhanced `micro.md`).
- **`--stem`**: prefix for `micro.md`, `micro_ratio.md`, chart filenames `micro_*.png`.

## CI hook (optional)

Example addition after an existing `-p4` step in `.github/workflows/release.yml`:

```yaml
      - name: Enhanced MICRO views (optional)
        if: matrix.platform == 'linux' && matrix.name == 'linux-x86_64'
        run: |
          python3 -m pip install -r scripts/requirements.txt
          ./turbobench blaris/bench/data/* -U -eMICRO -aMICRO
          ./scripts/enhance_bench_md.sh data@MICRO.tbb --stem micro --out-dir turbobench_ --skip-pip
```

Commit step (separate job or same bot commit as `benchmark.md`):

```yaml
          git add turbobench_/micro*.md turbobench_/charts/
```

For **Silesia** / root `benchmark.md`, extend the same pattern per platform artifact (`--stem silesia` etc.) before concatenation; multi-section `benchmark.md` may need a dedicated `--stem` per platform in a follow-up.

## Dependencies

- Python 3.10+
- `matplotlib`, `adjustText` (`scripts/requirements.txt`)

No link to the TurboBench compile except when you pass a `.tbb` file to `enhance_bench_md.sh`.