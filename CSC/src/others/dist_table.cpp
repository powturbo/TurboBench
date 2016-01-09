#include <cstdio>
#include <stdint.h>


static const uint32_t dist_table_[33] = {
        0,      1,      2,      3,
        5,      9,      17,     33,
        65,     129,    257,    513,
        1025,   2049,   4097,   8193,
        16385,  32769,  65537,  131073,
        262145, 524289, 1048577,        2097153,
        4194305,        8388609,        16777217,       33554433,
        67108865,       134217729,      268435457,      536870913,
        1073741825,
    };

int main()
{
    uint32_t last_dist = 0;
    for(uint32_t i = 0; i <= 32; i++) {
        uint32_t dist = 0;
        uint32_t extra = 0;
        if (i < 3) {
            dist = i;
            last_dist = dist;
        } else {
            extra = i - 2;
            dist = last_dist + (1 << extra);
        }
        //printf("%3u\t%2u bits\t%9u - %9u\n", i, extra, i < 3? last_dist : last_dist + 1, dist);
        printf("%u,\t", i < 3? last_dist : last_dist + 1);
        if ((i + 1) % 4 == 0) printf("\n");
        last_dist = dist;
    }

    printf("\n\n");

    for(uint32_t i = 0; i < 16; i++) {
        uint32_t r = 0;
        if (i & 1) r += 8;
        if (i & 2) r += 4;
        if (i & 4) r += 2;
        if (i & 8) r += 1;
        printf("%u,\t", r);
        if ((i + 1) % 4 == 0) printf("\n");
    }

    printf("\n\n");
    uint32_t dist;
    while(scanf("%u", &dist) != EOF) {
        uint32_t l = 0, r = 32, mid;
        while(l + 1 < r) {
            uint32_t mid = (l + (r - l) / 2);
            if (dist_table_[mid] > dist) 
                r = mid;
            else if (dist_table_[mid] < dist) 
                l = mid;
            else 
                l = r = mid;
        }
        uint32_t slot = l;

        uint32_t extra_len = 0, extra_bits = slot > 2? slot - 2 : 0;
        if (extra_bits) {
            extra_len = dist - (1 << extra_bits) - 1;
        }
        printf("%u -> slot: %u, extra_bits %u, extra_len: %u\n", dist, slot, extra_bits, extra_len);
    }
    return 0;
}
