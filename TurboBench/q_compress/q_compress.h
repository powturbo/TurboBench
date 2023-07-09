typedef struct FfiVec_ {
  void *raw_box;
  void *ptr;
  unsigned len;
} FfiVec;

#ifdef __cplusplus
extern "C" {
#endif
 
FfiVec auto_compress_i32(int *nums, unsigned len, unsigned level);
FfiVec auto_compress_i64(long long *nums, unsigned len, unsigned level);
void free_compressed(FfiVec ffi_vec);
FfiVec auto_decompress_i32( unsigned char *compressed, unsigned len );
FfiVec auto_decompress_i64( unsigned char *compressed, unsigned len );
void free_i32(FfiVec c_vec);

#ifdef __cplusplus
}
#endif


