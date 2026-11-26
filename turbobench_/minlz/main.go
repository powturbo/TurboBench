// C bindings for github.com/minio/minlz (raw MinLZ block format).
//
//	go build -buildmode=c-shared  -o libmlz.so .
//	go build -buildmode=c-archive -o libmlz.a .
package main

/*
#include <stdint.h>
#include <stddef.h>
*/
import "C"

import (
	"unsafe"

	"github.com/minio/minlz"
)

// Level used by mlzcompress.
// minlz.LevelSuperFast(-1) | LevelUncompressed(0) |
// LevelFastest(1) | LevelBalanced(2) | LevelSmallest(3)
const mlzLevel = minlz.LevelBalanced

//export mlzcompress
func mlzcompress(in *C.uint8_t, inlen C.size_t, out *C.uint8_t, outsize C.size_t) C.size_t {
	return compress(in, inlen, out, outsize, C.int(mlzLevel))
}

//export mlzcompresslevel
func mlzcompresslevel(in *C.uint8_t, inlen C.size_t, out *C.uint8_t, outsize C.size_t, level C.int) C.size_t {
	return compress(in, inlen, out, outsize, level)
}

func compress(in *C.uint8_t, inlen C.size_t, out *C.uint8_t, outsize C.size_t, level C.int) C.size_t {
	src, ok := asSlice(in, inlen)
	if !ok {
		return 0
	}
	dst, ok := asSlice(out, outsize)
	if !ok {
		return 0
	}
	enc, err := minlz.Encode(dst[:0], src, int(level))
	if err != nil {
		return 0
	}
	return C.size_t(finish(dst, enc))
}

//export mlzdecompress
func mlzdecompress(in *C.uint8_t, inlen C.size_t, out *C.uint8_t, outlen C.size_t) C.size_t {
	src, ok := asSlice(in, inlen)
	if !ok {
		return 0
	}
	dst, ok := asSlice(out, outlen)
	if !ok {
		return 0
	}
	// Cheap pre-check from the block header: reject before allocating for
	// a block that wants more room than the caller provided.
	if n, err := minlz.DecodedLen(src); err != nil || n > len(dst) {
		return 0
	}
	dec, err := minlz.Decode(dst[:0], src)
	if err != nil {
		return 0
	}
	return C.size_t(finish(dst, dec))
}

//export mlzdeclen
func mlzdeclen(in *C.uint8_t, inlen C.size_t) C.size_t {
	src, ok := asSlice(in, inlen)
	if !ok {
		return 0
	}
	n, err := minlz.DecodedLen(src)
	if err != nil {
		return 0
	}
	return C.size_t(n)
}

//export mlzmaxencodedlen
func mlzmaxencodedlen(srclen C.size_t) C.size_t {
	if uint64(srclen) > uint64(^uint(0)>>1) {
		return 0
	}
	n := minlz.MaxEncodedLen(int(srclen))
	if n <= 0 {
		return 0
	}
	return C.size_t(n)
}

// ---- glue helpers ------------------------------------------------------

// asSlice wraps a C (pointer, length) pair into a Go byte slice without
// copying. ok=false for NULL/zero-length/oversized buffers.
func asSlice(p *C.uint8_t, n C.size_t) ([]byte, bool) {
	if p == nil || n == 0 {
		return nil, false
	}
	if uint64(n) > uint64(^uint(0)>>1) {
		return nil, false
	}
	return unsafe.Slice((*byte)(unsafe.Pointer(p)), int(n)), true
}

// finish returns len(res), guaranteeing the bytes live in dst.
// If minlz allocated its own buffer (res didn't fit in dst's capacity),
// the data is copied back when it nevertheless fits, otherwise this is
// reported as an error (0).
func finish(dst, res []byte) int {
	if len(res) == 0 || len(res) > len(dst) {
		return 0
	}
	if !within(dst, res) {
		copy(dst, res) // foreign allocation → move into the caller's buffer
	}
	return len(res)
}

// within reports whether res is backed by dst's memory.
func within(dst, res []byte) bool {
	if len(res) == 0 {
		return true
	}
	d0 := uintptr(unsafe.Pointer(&dst[0]))
	r0 := uintptr(unsafe.Pointer(&res[0]))
	return r0 >= d0 && r0+uintptr(len(res)) <= d0+uintptr(cap(dst))
}

func main() {}
