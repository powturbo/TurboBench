// ============================================================================
// D2-prime: LANE-PER-BLOCK parse + WARP-COOPERATIVE copy.
//
// Why this is allowed: blocks are ALREADY rep-independent by construction --
// BLOCK_MARKER (0xFF) resets rep[4] at the start of every block. So a lane can
// own an entire block, keep its own rep chain intact, and lose nothing.
//
// Phase A: each of the 32 lanes parses ITS OWN block, buffering up to Q tokens
//          as (src, dst, len, isLit) triplets in shared memory.
// Phase B: the whole warp walks the buffered triplets and copies each one with
//          all 32 lanes striding across its bytes -- the same inner loop as the
//          shipping kernel, so the match-length curve still works for us.
//
// Format unchanged. Compression ratio unchanged. Bit-perfect required.
//
// Measured motivation (tuned FASTQ 1GB, H100):
//   full pipeline            4.478 ms  (239.8 GB/s)
//   parse-only, group/block  2.848 ms
//   parse-only, LANE/block   1.286 ms   <-- parse 2.2x faster
//   copy (by difference)     1.630 ms
//   => projected 0.40 (ANS) + 0.89 (parse) + 1.63 (copy) = 2.92 ms ~ 368 GB/s
// ============================================================================
#pragma once

#define D2P_Q 8   // tokens buffered per lane per round

struct Trip {
    uint32_t dst;    // absolute position in the output buffer
    uint32_t src;    // match: absolute position in output. literal: index into LIT
    uint32_t len;
    uint32_t isLit;
};

__global__ void k_decode_d2p(const uint8_t* __restrict__ LIT, const uint8_t* __restrict__ OFF,
                             const uint8_t* __restrict__ LEN, const uint8_t* __restrict__ CMD,
                             const BlockOffsets* __restrict__ boffs, uint32_t num_blocks,
                             uint64_t orig_size, uint32_t block_size, uint8_t* __restrict__ out,
                             uint32_t* __restrict__ blk_ctr, uint32_t blk_end)
{
    extern __shared__ Trip sh[];
    const uint32_t lane     = threadIdx.x & 31;
    const uint32_t warp_id  = threadIdx.x >> 5;
    Trip* T = sh + (size_t)warp_id * (32 * D2P_Q);

    for (;;) {
        // one warp claims 32 consecutive blocks: lane j takes block base+j
        uint32_t base = 0;
        if (lane == 0) base = atomicAdd(blk_ctr, 32u);
        base = __shfl_sync(0xffffffffu, base, 0);
        if (base >= blk_end) return;

        const uint32_t b   = base + lane;
        const bool active  = (b < blk_end);

        // ---- per-lane parser state for ITS OWN block ----
        uint32_t lit_base=0, off_base=0, len_base=0, cmd_base=0;
        uint32_t lit_sz=0, off_sz=0, len_sz=0, cmd_sz=0;
        uint32_t lp=0, op=0, np=0, cp=0;      // stream cursors (block-relative)
        uint32_t out_pos=0, dst_size=0;
        uint64_t bbase=0;
        uint32_t rep[4] = {1,2,4,8};

        if (active) {
            BlockOffsets bo = boffs[b];
            lit_base=(uint32_t)bo.lit_off; off_base=(uint32_t)bo.off_off;
            len_base=(uint32_t)bo.len_off; cmd_base=(uint32_t)bo.cmd_off;
            lit_sz=(uint32_t)bo.lit_sz; off_sz=(uint32_t)bo.off_sz;
            len_sz=(uint32_t)bo.len_sz; cmd_sz=(uint32_t)bo.cmd_sz;
            bbase = (uint64_t)b * block_size;
            uint64_t rem = orig_size - bbase;
            dst_size = (uint32_t)((rem < (uint64_t)block_size) ? rem : (uint64_t)block_size);
        }

        const uint8_t* lit = LIT + lit_base;
        const uint8_t* off = OFF + off_base;
        const uint8_t* len = LEN + len_base;
        const uint8_t* cmd = CMD + cmd_base;

        // ---- rounds ----
        for (;;) {
            uint32_t n = 0;

            // ===== PHASE A: this lane parses its own block =====
            if (active) {
                while (n < D2P_Q && out_pos < dst_size && cp < cmd_sz) {
                    uint8_t c = cmd[cp++];
                    if (c == 0xFF) { rep[0]=1; rep[1]=2; rep[2]=4; rep[3]=8; continue; }

                    uint32_t l = 0, aux = 0, isLit = 0;

                    if (c < 0x80) {                                   // literal run
                        l = (uint32_t)c + 1;
                        if (lp + l > lit_sz || out_pos + l > dst_size) { out_pos = dst_size; break; }
                        isLit = 1; aux = lp; lp += l;
                    } else if ((c & 0xC0) == 0x80) {                  // repeat-offset match
                        uint32_t ri = (c >> 4) & 3, lv = c & 0x0F;
                        if (lv == 0x0F) lv += rd_varint(len, np, len_sz);
                        l = lv + 6;
                        uint32_t dist = rep[ri];
                        if (ri > 0) { for (int i=(int)ri; i>0; i--) rep[i]=rep[i-1]; rep[0]=dist; }
                        if (!dist || dist > out_pos || out_pos + l > dst_size) { out_pos = dst_size; break; }
                        aux = dist;
                    } else {                                          // explicit-offset match
                        uint32_t lv = (c == 0xFE) ? rd_varint(len, np, len_sz) : (uint32_t)(c & 0x3F);
                        l = lv + 6;
                        uint32_t dist = rd_varint(off, op, off_sz);
                        rep[3]=rep[2]; rep[2]=rep[1]; rep[1]=rep[0]; rep[0]=dist;
                        if (!dist || dist > out_pos || out_pos + l > dst_size) { out_pos = dst_size; break; }
                        aux = dist;
                    }

                    Trip t;
                    t.isLit = isLit;
                    t.len   = l;
                    t.dst   = (uint32_t)(bbase + out_pos);
                    t.src   = isLit ? (lit_base + aux)                       // absolute index into LIT
                                    : (uint32_t)(bbase + out_pos - aux);     // absolute position in out
                    T[lane * D2P_Q + n] = t;
                    n++;
                    out_pos += l;
                }
            }
            __syncwarp();

            // how many tokens did every lane produce?
            uint32_t total = 0;
            for (int j = 0; j < 32; j++) total += __shfl_sync(0xffffffffu, n, j);
            if (total == 0) break;

            // ===== PHASE B: whole warp copies the buffered triplets =====
            for (int j = 0; j < 32; j++) {
                uint32_t nj = __shfl_sync(0xffffffffu, n, j);
                for (uint32_t k = 0; k < nj; k++) {
                    Trip t = T[j * D2P_Q + k];
                    if (t.isLit) {
                        for (uint32_t i = lane; i < t.len; i += 32)
                            out[t.dst + i] = LIT[t.src + i];
                    } else {
                        uint32_t d = t.dst - t.src;                   // match distance
                        if (d >= t.len) {
                            for (uint32_t i = lane; i < t.len; i += 32)
                                out[t.dst + i] = out[t.src + i];
                        } else {                                      // overlapping match
                            for (uint32_t i = lane; i < t.len; i += 32)
                                out[t.dst + i] = out[t.src + (i % d)];
                        }
                    }
                }
            }
            __syncwarp();
        }
    }
}
