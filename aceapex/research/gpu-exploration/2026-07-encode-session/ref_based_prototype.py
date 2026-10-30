#!/usr/bin/env python3
# ============================================================================
# ref_based_prototype.py — измеряет РЕАЛЬНЫЙ ratio reference-based кодирования
# на настоящих данных пода (NA12878 FASTQ reads против hg38 chr1), НЕ синтетика.
#
# ЦЕЛЬ: проверить идею №1 из research-directions — даёт ли absolute-offset
# структура ACEAPEX уникальную возможность reference-based сжатия геномных
# reads, и какой порядок выигрыша по ratio против self-referential (текущий
# ACEAPEX/zstd). Это кандидат на "новый класс по ratio".
#
# ЧЕСТНЫЕ ГРАНИЦЫ (заявлены заранее, до измерения):
#   - Это ПОТОЛОК-оценка: энкодер здесь получает выравнивание почти даром
#     (простой k-mer seed), а реальная стоимость reference-based = alignment
#     (это encode-цена, отдельный вопрос). Мы меряем RATIO-потолок, не encode-скорость.
#   - CRAM уже делает reference-based геномику. Новизна ACEAPEX НЕ в идее, а в
#     том, что это общий absolute-offset LZ-механизм, GPU-декодируемый, с
#     position-invariant random access. Этот скрипт меряет только ratio-механизм.
#   - reads с indels обрабатываются грубо (только замены на выровненном окне);
#     реальный выигрыш может быть нибольше/ниже — это первая оценка.
#
# ЗАПУСК на поде:
#   python3 ref_based_prototype.py /workspace/data/hg38/chr1.fa /workspace/data/NA12878_1gb.fastq
#
# ВЫВОД: ratio self-referential (zstd) vs reference-based (absolute-offset),
#   на подвыборке reads (для скорости), + экстраполяция.
# ============================================================================
import sys, subprocess, random, math
from collections import Counter

def read_reference(path, max_bp=5_000_000):
    seq = []
    with open(path) as f:
        for line in f:
            if line.startswith('>'): continue
            seq.append(line.strip().upper())
            if sum(len(s) for s in seq) >= max_bp: break
    ref = ''.join(seq)[:max_bp]
    # только ACGT (референс может иметь N)
    return ref

def read_fastq_reads(path, n_reads=50000):
    reads = []
    with open(path) as f:
        i = 0
        for line in f:
            if i % 4 == 1:  # строка последовательности
                s = line.strip().upper()
                if all(c in 'ACGT' for c in s):
                    reads.append(s)
            i += 1
            if len(reads) >= n_reads: break
    return reads

def build_kmer_index(ref, k=20):
    idx = {}
    for i in range(len(ref)-k+1):
        idx.setdefault(ref[i:i+k], i)  # первая позиция достаточно для seed
    return idx

def align_read(read, ref, kmer_idx, k=20):
    # seed: первый k-mer read, ищем в референсе -> кандидат позиции
    if len(read) < k: return None
    seed = read[:k]
    pos = kmer_idx.get(seed)
    if pos is None: return None
    if pos + len(read) > len(ref): return None
    ref_sub = ref[pos:pos+len(read)]
    diffs = [(i, read[i]) for i in range(len(read)) if read[i] != ref_sub[i]]
    return pos, diffs

def ref_based_bits(pos, diffs, ref_len, read_len):
    bits = math.ceil(math.log2(ref_len))       # absolute position в референс
    bits += 5                                    # число правок (0-31)
    bits += len(diffs) * (math.ceil(math.log2(read_len)) + 2)  # позиция+символ
    return bits

def zstd_ratio(data_bytes):
    try:
        p = subprocess.run(['zstd','-19','-c'], input=data_bytes,
                           capture_output=True)
        return len(data_bytes)/len(p.stdout) if p.stdout else None
    except FileNotFoundError:
        import zlib
        c = zlib.compress(data_bytes, 9)
        return len(data_bytes)/len(c)

def main():
    if len(sys.argv) < 3:
        print("usage: ref_based_prototype.py <reference.fa> <reads.fastq>")
        sys.exit(1)
    ref_path, fq_path = sys.argv[1], sys.argv[2]
    print("Загрузка референса...")
    ref = read_reference(ref_path)
    print(f"  референс: {len(ref)/1e6:.1f} Mbp")
    print("Загрузка reads...")
    reads = read_fastq_reads(fq_path)
    print(f"  reads: {len(reads)} (для скорости прототипа)")
    if not reads:
        print("нет ACGT-reads — проверь FASTQ формат"); sys.exit(1)
    rl = len(reads[0])
    print(f"  длина read: {rl} bp")
    print("Индекс k-mer референса (может занять минуту)...")
    kidx = build_kmer_index(ref, k=20)
    print(f"  {len(kidx)} уникальных 20-mer")
    print()

    # A) self-referential: zstd-19 на сырых reads
    raw = ''.join(reads).encode()
    r_self = zstd_ratio(raw)
    print(f"A) Self-referential (zstd-19, аналог относительного LZ): ratio = {r_self:.2f}x")

    # B) reference-based через absolute offsets
    aligned = 0; total_bits = 0; unaligned_bits = 0
    for read in reads:
        res = align_read(read, ref, kidx, k=20)
        if res is None:
            # не выровнялся -> храним как есть (2 бита/база)
            unaligned_bits += len(read)*2
        else:
            pos, diffs = res
            total_bits += ref_based_bits(pos, diffs, len(ref), len(read))
            aligned += 1
    comp_bits = total_bits + unaligned_bits
    r_ref = (len(raw)*8) / comp_bits
    print(f"B) Reference-based (absolute offsets в hg38, референс у получателя):")
    print(f"     выровнялось: {aligned}/{len(reads)} reads ({100*aligned/len(reads):.1f}%)")
    print(f"     ratio = {r_ref:.2f}x  <- геномный сценарий (hg38 стандартен)")
    print()
    if r_self:
        print(f"ВЫИГРЫШ reference-based vs self-referential: {r_ref/r_self:.1f}x плотнее")
    print()
    print("ГРАНИЦА: это ratio-ПОТОЛОК. encode-цена (alignment) — отдельно.")
    print("Реальная ценность ACEAPEX: этот поток GPU-декодируем с random access,")
    print("чего у CRAM нет. Следующий шаг если ratio высокий — reference-режим в кодек.")

if __name__ == '__main__':
    main()
