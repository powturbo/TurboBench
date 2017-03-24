#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
uint8_t * bbcompress( const uint8_t * const data, const long size, uint8_t * new_data,
                      long * const out_sizep, int dictionary_size, int match_len_limit );
uint8_t * bbdecompress( const uint8_t * const data, const long size, uint8_t * new_data,
                        long * const out_sizep );
#ifdef __cplusplus
} /* extern "C" */
#endif
						
					  
