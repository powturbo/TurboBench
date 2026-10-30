#!/usr/bin/env python3
"""Figures of docs/paper_biorxiv/paper.md, read from the run logs in results/ (no number typed in by hand).
Usage: python3 docs/paper_biorxiv/make_figures.py   (from the repository root; writes docs/paper_biorxiv/figures/*.png)"""
import re
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

R = "results/"
OUT = "docs/paper_biorxiv/figures/"
H100 = R + "runpod-2026-10-02-h100-80gb-hbm3-gpu-834d5d1.log"
BW = R + "colab-2026-10-02-rtx-pro-6000-blackwell-server-edition-gpu.log"
SWEEP = R + "h100-pcie-sweep-2026-10-03.log"


def lines(path):
    with open(path, "rb") as f:
        return f.read().decode("utf-8", "replace").splitlines()


def h5rows(path):
    rows = []
    for l in lines(path):
        if l.startswith("H5ROW\t"):
            c = l.split("\t")
            rows.append(dict(W=int(c[1]), n=int(c[2]), wps=float(c[3]), gbs=float(c[4]), cpu=float(c[10])))
    return rows


def fig_loader():
    fig, ax = plt.subplots(1, 2, figsize=(9, 3.4))
    for path, name, mk in ((BW, "RTX PRO 6000 Blackwell", "o"), (H100, "H100 80GB HBM3", "s")):
        rows = h5rows(path)
        for W, ls in ((1024, "-"), (8192, "--"), (32768, ":")):
            r = sorted([x for x in rows if x["W"] == W], key=lambda x: x["n"])
            ax[0].plot([x["n"] for x in r], [x["wps"] / 1e6 for x in r], ls, marker=mk, label=f"{name}, {W // 1024} KiB")
            ax[1].plot([x["n"] for x in r], [x["gbs"] for x in r], ls, marker=mk)
    for a in ax:
        a.set_xscale("log", base=2); a.set_xlabel("windows per batch"); a.grid(alpha=.3)
    ax[0].set_ylabel("windows / s (millions)"); ax[1].set_ylabel("window bytes / s (GB/s)")
    ax[0].legend(fontsize=6.5)
    fig.tight_layout(); fig.savefig(OUT + "fig1_loader.png", dpi=200); plt.close(fig)


def fig_pcie():
    pat = re.compile(r"pieces of (\d+) MB .*?= ([\d.]+) GB/s of output = ([\d.]+)x the bus \(H2D ([\d.]+) GB/s\); \+ D2H [\d.]+ s = ([\d.]+) GB/s")
    res = re.compile(r"decode resident [\d.]+ ms = ([\d.]+) GB/s")
    pts, ceil = [], []
    for l in lines(SWEEP):
        m = pat.search(l)
        if m: pts.append(tuple(float(x) for x in m.groups()))
        m = res.search(l)
        if m: ceil.append(float(m.group(1)))
    fig, ax = plt.subplots(figsize=(5.2, 3.4))
    ax.plot([p[0] for p in pts], [p[1] for p in pts], "o-", label="H2D -> decode, output on the card")
    ax.plot([p[0] for p in pts], [p[4] for p in pts], "s--", label="same + D2H of the output")
    ax.plot([p[0] for p in pts], [p[3] for p in pts], "k:", label="H2D of the archive (bus)")
    ax.axhline(max(ceil), color="gray", lw=1, label=f"decode, archive resident ({max(ceil):.1f})")
    ax.set_xscale("log", base=2); ax.set_xlabel("piece of the archive sent per step (MB)"); ax.set_ylabel("GB/s of decoded output")
    ax.set_ylim(0, 180); ax.grid(alpha=.3); ax.legend(fontsize=7)
    fig.tight_layout(); fig.savefig(OUT + "fig2_pcie.png", dpi=200); plt.close(fig)


def fig_stress():
    pat = re.compile(r"^\[stress\]\s+(bit flip|random bytes|zeroed run|truncation|header/table|literal stream|token streams)\s+(\d+) /\s+(\d+) /\s+(\d+) /\s+(\d+)")
    kinds, vals = [], []
    for l in lines(BW):
        m = pat.search(l)
        if m: kinds.append(m.group(1)); vals.append([int(x) for x in m.groups()[1:]])
    fig, ax = plt.subplots(figsize=(6.4, 3.4))
    lab = ["refused by the plan", "caught on the device", "harmless", "silent without hash"]
    col = ["#4c72b0", "#55a868", "#bbbbbb", "#c44e52"]
    bottom = [0] * len(kinds)
    for j in range(4):
        v = [x[j] for x in vals]
        ax.bar(kinds, v, bottom=bottom, label=lab[j], color=col[j])
        bottom = [a + b for a, b in zip(bottom, v)]
    ax.set_ylabel("corrupt archives"); ax.tick_params(axis="x", rotation=30, labelsize=7); ax.legend(fontsize=7)
    ax.set_title("T-H4, 10 000 corrupt copies; with XXH3: silent 0, hangs 0", fontsize=8)
    fig.tight_layout(); fig.savefig(OUT + "fig3_stress.png", dpi=200); plt.close(fig)


if __name__ == "__main__":
    fig_loader(); fig_pcie(); fig_stress()
