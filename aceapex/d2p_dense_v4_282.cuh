// ============================================================================
// D1-dense: two-kernel decode with a DENSE, ORDERED triplet array.
//
// Kernel A  (lane per block): each lane parses its own block. rep[4] chain stays
//           intact -- blocks are already rep-independent (0xFF resets rep).
//           Triplets are buffered in shared, then flushed to global with a
//           WARP-TRANSPOSED write so the stores coalesce.
//
// Kernel B  (warp per block): the warp reads that block's contiguous triplet run
//           and copies each token in order, 32 lanes striding across its bytes --
//           exactly the inner loop, and exactly the locality, of the fused kernel.
//
// Dense index, for free: every token consumes one byte of the CMD stream, and the
// position of that byte is monotone in token order. So a token whose command byte
// sits at offset p inside block b gets slot  cmd_off[b] + p.  BlockOffsets.cmd_off
// is already the running prefix sum across blocks -- no scan, no counting pass.
// 0xFF markers simply leave an empty slot (len = 0), which kernel B skips.
//
// Array size = total CMD bytes (27.4 MB here) x 16 B = 438 MB, dense,
// versus 2.15 GB at 92% holes for the block-major layout that lost us 0.78 ms.
//
// Access-pattern check before predicting (the rule that would have caught our
// four wrong predictions):
//   parse : same pattern as the measured lane-per-block ablation  -> 0.79 ms
//   write : same pattern as the measured warp-contiguous flush    -> 0.10 ms
//   copy  : same pattern as the fused kernel                      -> 1.63 ms
//   ANS   : measured                                              -> 0.50 ms
//   predicted total 3.0-3.4 ms  (baseline 4.46 ms / 240.9 GB/s)
// Pass/fail set BEFORE the run: kernel A <= 1.7 ms incl. write; A+B+ANS <= 3.7 ms.
// ============================================================================
#pragma once

#define D2P_Q 32   // triplets buffered per lane before a coalesced flush

// PACKED TOKEN, 4 bytes instead of 16.
//   dst is REDUNDANT: kernel B walks a block's tokens in order and accumulates
//   out_pos itself (one add per token, free -- it is already serial over tokens).
//   aux  (match distance, or literal offset inside the block) < block_size = 16384 -> 14 bits
//   len  (bounded by block_size)                                          <= 16384 -> 15 bits
//   isLit                                                                          ->  1 bit
//   total 30 bits. Traffic drops 878 MB -> 220 MB, and the store transaction count
//   drops 4x as well (8 packed tokens of one lane = 32 B = exactly one sector).
typedef uint32_t D2PTrip;
#define D2P_PACK(len,aux,isLit)  ( ((uint32_t)(len) << 15) | ((uint32_t)(aux) << 1) | (uint32_t)(isLit) )
#define D2P_LEN(t)   ((t) >> 15)
#define D2P_AUX(t)   (((t) >> 1) & 0x3FFFu)
#define D2P_ISLIT(t) ((t) & 1u)

// ---------------------------------------------------------------------------
// Kernel A: lane-per-block parse -> dense triplets, coalesced via shared transpose
// ---------------------------------------------------------------------------
__global__ void k_parse_d2p(const uint8_t* __restrict__ LIT, const uint8_t* __restrict__ OFF,
                            const uint8_t* __restrict__ LEN, const uint8_t* __restrict__ CMD,
                            const BlockOffsets* __restrict__ boffs, uint32_t num_blocks,
                            uint64_t orig_size, uint32_t block_size,
                            D2PTrip* __restrict__ TRIP, uint32_t* __restrict__ blk_ctr)
{
    extern __shared__ unsigned char d2p_smem[];
    D2PTrip* sh = reinterpret_cast<D2PTrip*>(d2p_smem);
    const uint32_t lane    = threadIdx.x & 31;
    const uint32_t warp_id = threadIdx.x >> 5;
    D2PTrip* T   = sh + (size_t)warp_id * (32 * D2P_Q);   // this warp's buffer
    uint32_t* SL = reinterpret_cast<uint32_t*>(sh + (size_t)(blockDim.x/32) * (32 * D2P_Q));
    uint32_t* slot = SL + (size_t)warp_id * (32 * D2P_Q); // slot index for each buffered triplet

    for (;;) {
        uint32_t base = 0;
        if (lane == 0) base = atomicAdd(blk_ctr, 32u);
        base = __shfl_sync(0xffffffffu, base, 0);
        if (base >= num_blocks) return;

        const uint32_t b  = base + lane;
        const bool active = (b < num_blocks);

        uint32_t lit_base=0, off_base=0, len_base=0, cmd_base=0;
        uint32_t lit_sz=0, off_sz=0, len_sz=0, cmd_sz=0;
        uint32_t lp=0, op=0, np=0, cp=0, out_pos=0, dst_size=0;
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

        for (;;) {
            uint32_t n = 0;

            // ---- parse up to Q tokens from THIS lane's block ----
            if (active) {
                while (n < D2P_Q && out_pos < dst_size && cp < cmd_sz) {
                    uint32_t cmd_pos = cp;               // slot is keyed on the command byte
                    uint8_t c = cmd[cp++];
                    if (c == 0xFF) { rep[0]=1; rep[1]=2; rep[2]=4; rep[3]=8; continue; }

                    uint32_t l=0, aux=0, isLit=0;
                    if (c < 0x80) {
                        l = (uint32_t)c + 1;
                        if (lp + l > lit_sz || out_pos + l > dst_size) { out_pos = dst_size; break; }
                        isLit = 1; aux = lp; lp += l;
                    } else if ((c & 0xC0) == 0x80) {
                        uint32_t ri = (c >> 4) & 3, lv = c & 0x0F;
                        if (lv == 0x0F) lv += rd_varint(len, np, len_sz);
                        l = lv + 6;
                        uint32_t dist = rep[ri];
                        if (ri > 0) { for (int i=(int)ri;i>0;i--) rep[i]=rep[i-1]; rep[0]=dist; }
                        if (!dist || dist > out_pos || out_pos + l > dst_size) { out_pos = dst_size; break; }
                        aux = dist;
                    } else {
                        uint32_t lv = (c == 0xFE) ? rd_varint(len, np, len_sz) : (uint32_t)(c & 0x3F);
                        l = lv + 6;
                        uint32_t dist = rd_varint(off, op, off_sz);
                        rep[3]=rep[2]; rep[2]=rep[1]; rep[1]=rep[0]; rep[0]=dist;
                        if (!dist || dist > out_pos || out_pos + l > dst_size) { out_pos = dst_size; break; }
                        aux = dist;
                    }

                    // aux is BLOCK-LOCAL: literal offset within this block's LIT run,
                    // or match distance (always < out_pos <= block_size).
                    D2PTrip t = D2P_PACK(l, aux, isLit);
                    T[lane * D2P_Q + n]    = t;
                    slot[lane * D2P_Q + n] = cmd_base + cmd_pos;   // DENSE, ORDERED index
                    n++;
                    out_pos += l;
                }
            }
            __syncwarp();

            uint32_t total = 0;
            for (int j = 0; j < 32; j++) total += __shfl_sync(0xffffffffu, n, j);
            if (total == 0) break;

            // ---- coalesced flush: warp drains one lane's run at a time, so the 32
            //      lanes write 32 CONSECUTIVE elements of that run per step.
            //      Destination is the dense cmd_off layout; only the ORDER of stores
            //      is transposed, which is what makes them coalesce.
            for (int j = 0; j < 32; j++) {
                uint32_t nj = __shfl_sync(0xffffffffu, n, j);
                for (uint32_t k = lane; k < nj; k += 32) {
                    TRIP[ slot[j * D2P_Q + k] ] = T[j * D2P_Q + k];
                }
            }
            __syncwarp();
        }
    }
}

// ---------------------------------------------------------------------------
// Kernel B: warp per block, copies that block's dense run in order
// ---------------------------------------------------------------------------
__global__ void k_copy_d2p(const uint8_t* __restrict__ LIT,
                           const D2PTrip* __restrict__ TRIP,
                           const BlockOffsets* __restrict__ boffs,
                           uint32_t num_blocks, uint32_t block_size,
                           uint8_t* __restrict__ out,
                           uint32_t* __restrict__ blk_ctr)
{
    const uint32_t lane = threadIdx.x & 31;
    for (;;) {
        uint32_t b = 0;
        if (lane == 0) b = atomicAdd(blk_ctr, 1u);
        b = __shfl_sync(0xffffffffu, b, 0);
        if (b >= num_blocks) return;

        BlockOffsets bo = boffs[b];
        const D2PTrip* run = TRIP + bo.cmd_off;      // contiguous run for this block
        uint32_t nt = (uint32_t)bo.cmd_sz;           // one slot per command byte
        uint64_t bbase   = (uint64_t)b * block_size;
        uint32_t lit_base= (uint32_t)bo.lit_off;
        uint32_t out_pos = 0;                        // reconstructed, not stored
        uint32_t lp      = 0;                        // literal cursor, also reconstructed

        // Batched: the warp loads 32 tokens with ONE coalesced load, then copies them
        // in order. Before: 32 lanes each broadcast-loaded the same 4-byte token and hit
        // __syncwarp once per token -- 27.4M warp syncs across the file.
        for (uint32_t k0 = 0; k0 < nt; k0 += 32) {
            uint32_t idx = k0 + lane;
            D2PTrip tk = (idx < nt) ? run[idx] : 0u;      // ONE coalesced 128-byte load

            // running out_pos / lit cursor across this batch: warp-inclusive prefix sums
            uint32_t l_i  = D2P_LEN(tk);
            uint32_t ll_i = D2P_ISLIT(tk) ? l_i : 0u;
            uint32_t pre_l = l_i, pre_ll = ll_i;
            #pragma unroll
            for (int d = 1; d < 32; d <<= 1) {
                uint32_t a1 = __shfl_up_sync(0xffffffffu, pre_l,  d);
                uint32_t a2 = __shfl_up_sync(0xffffffffu, pre_ll, d);
                if ((int)lane >= d) { pre_l += a1; pre_ll += a2; }
            }
            uint32_t my_out = out_pos + pre_l  - l_i;     // exclusive prefix
            uint32_t my_lp  = lp      + pre_ll - ll_i;

            uint32_t batch_len = __shfl_sync(0xffffffffu, pre_l,  31);
            uint32_t batch_lit = __shfl_sync(0xffffffffu, pre_ll, 31);

            uint32_t nb_tok = (nt - k0 < 32) ? (nt - k0) : 32;
            for (uint32_t j = 0; j < nb_tok; j++) {
                D2PTrip t = __shfl_sync(0xffffffffu, tk,     j);
                uint32_t l = __shfl_sync(0xffffffffu, l_i,   j);
                if (l == 0) continue;
                uint32_t o = __shfl_sync(0xffffffffu, my_out, j);
                uint32_t p = __shfl_sync(0xffffffffu, my_lp,  j);
                uint32_t aux = D2P_AUX(t);
                uint32_t dstp = (uint32_t)(bbase + o);
                if (D2P_ISLIT(t)) {
                    for (uint32_t i = lane; i < l; i += 32)
                        out[dstp + i] = LIT[lit_base + p + i];
                } else {
                    uint32_t srcp = dstp - aux;
                    if (aux >= l) {
                        for (uint32_t i = lane; i < l; i += 32)
                            out[dstp + i] = out[srcp + i];
                    } else {
                        for (uint32_t i = lane; i < l; i += 32)
                            out[dstp + i] = out[srcp + (i % aux)];
                    }
                }
                __syncwarp();
            }
            out_pos += batch_len;
            lp      += batch_lit;
        }
    }
}
