#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
uint8_t * bbcompress( const uint8_t * const data, const int size, uint8_t * new_data,
                      int * const out_sizep, int dicsize, int max_len );
uint8_t * bbdecompress( const uint8_t * const data, const int size, uint8_t * new_data,
                        int * const out_sizep );
#ifdef __cplusplus
} /* extern "C" */
#endif
						
					  
