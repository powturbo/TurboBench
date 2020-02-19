#ifdef __cplusplus
extern "C" {
#endif

#define FA_WORKMEM ((((size_t) 1) << 17) * sizeof(unsigned short))

int fa_compress(const unsigned char* ibuf, unsigned char* obuf, size_t ilen, size_t* olen, void* workmem);
int fa_decompress(const unsigned char* ibuf, unsigned char* obuf, size_t ilen, size_t* olen, void* workmem);

#ifdef __cplusplus
}
#endif

