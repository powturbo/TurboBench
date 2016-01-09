#include <cstdio>
#include <unordered_map>
#include <stdint.h>
#include <vector>
#include <string>
#include <algorithm>


using namespace std;


#define BSIZE 10000000
int main(int argc, char *argv[])
{
    if (argc < 2) return 1;

    FILE *f = fopen(argv[1], "rb");
    if (f == NULL) return 1;


    unordered_map<string, uint32_t> m;
#define LC(x) (x >= 'a' && x <= 'z')

    uint8_t *buf = new uint8_t[BSIZE + 3];
    for(size_t p = 0;;) {
        size_t n = fread(buf, 1, BSIZE, f);
        for(size_t i = 0; i < n; i++) {
            if (LC(buf[i]) && LC(buf[i + 1])) {
                string key;
                key.append(1, buf[i]);
                key.append(1, buf[i + 1]);
            }
            if (LC(buf[i]) && LC(buf[i + 1]) && LC(buf[i + 2])) {
                string key;
                key.append(1, buf[i]);
                key.append(1, buf[i + 1]);
                key.append(1, buf[i + 2]);
                m[key] += 1;
            }
            if (LC(buf[i]) && LC(buf[i + 1]) 
                    && LC(buf[i + 2]) && LC(buf[i + 3])) {
                string key;
                key.append(1, buf[i]);
                key.append(1, buf[i + 1]);
                key.append(1, buf[i + 2]);
                key.append(1, buf[i + 3]);
                m[key] += 1;
            }
        }
        if (n < BSIZE)
            break;
        p += n;
        fprintf(stderr, "\r%u\t\t", p);
    }
    fclose(f);
    delete[] buf;

   fprintf(stderr, "\n%u keys got\n", m.size());
   vector<pair<string, uint32_t>> sorted;
   for(const auto &i : m) 
       sorted.push_back(i);

   sort(sorted.begin(), sorted.end(), [&](const auto& a, const auto& b) { return a.second > b.second; });
   for(int i = 0; i < 256; i++)
       printf("%3d: %6s\t\t%u\n", i, sorted[i].first.c_str(), sorted[i].second);

   printf("\n\n");
   for(int i = 1; i < 123; i++) {
       if ((i - 1) % 6 == 0) printf("\n");
       printf("\"%s\",   ", sorted[i - 1].first.c_str());
   }

   return 0;
}



