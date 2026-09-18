#ifdef __cplusplus
extern "C" {
#endif

size_t qlz_compress1(const void *source, char *destination, size_t size, char *scratch);
size_t qlz_decompress1(const char *source, void *destination, char *scratch);
int qlz_get_setting1(int setting);

size_t qlz_compress2(const void *source, char *destination, size_t size, char *scratch);
size_t qlz_decompress2(const char *source, void *destination, char *scratch);
int qlz_get_setting2(int setting);

size_t qlz_compress3(const void *source, char *destination, size_t size, char *scratch);
size_t qlz_decompress3(const char *source, void *destination, char *scratch);
int qlz_get_setting3(int setting);

#ifdef __cplusplus
}
#endif
