#ifdef __cplusplus
extern "C" {
#endif

void compress_block(unsigned char *in,  unsigned int in_size, unsigned char *out, unsigned int *out_size);
void uncompress_block(unsigned char *in, unsigned char *out, int out_size);
#ifdef __cplusplus
}
#endif
