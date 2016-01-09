#ifndef _CSA_INDEXPACK_H_
#define _CSA_INDEXPACK_H_
#include <stdint.h>
#include <csa_typedef.h>

char *Put8(uint64_t i, char *buf);
char *Get8(uint64_t& i, char *buf);
char *Get8(int64_t& i, char *buf);
char *Put4(uint32_t i, char *buf);
char *Get4(uint32_t& i, char *buf);
char *Put2(uint16_t i, char *buf);
char *Get2(uint64_t& i, char *buf);
char *PackIndex(FileIndex &fi, ABIndex &abi, uint64_t &total_size);
bool UnpackIndex(FileIndex &fi, ABIndex &abi, char *buf, uint64_t total_size);


#endif

