/**
 * zling:
 *  light-weight lossless data compression utility.
 *
 * Copyright (C) 2012-2013 by Zhang Li <zhangli10 at baidu.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @author zhangli10<zhangli10@baidu.com>
 * @brief  libzling utils.
 */
#ifndef SRC_LIBZLING_UTILS_MEM_H
#define SRC_LIBZLING_UTILS_MEM_H

#include "../libzling/src/libzling_inc.h"
#include "../libzling/src/libzling_utils.h"

namespace baidu {
namespace zling {


struct MemInputter: public baidu::zling::Inputter {
	MemInputter(uint8_t* buffer, size_t buflen) :
		m_buffer(buffer),
		m_buflen(buflen),
        m_total_read(0) {}

    size_t GetData(unsigned char* buf, size_t len);
    bool   IsEnd();
    bool   IsErr();
    size_t GetInputSize();

private:
	uint8_t* m_buffer;
	size_t m_buflen, m_total_read;
};

struct MemOutputter : public baidu::zling::Outputter {
	MemOutputter(uint8_t* buffer, size_t buflen) :
		m_buffer(buffer),
		m_buflen(buflen),
        m_total_write(0) {}

    size_t PutData(unsigned char* buf, size_t len);
    bool   IsErr();
    size_t GetOutputSize();

private:
    FILE*  m_fp;
	uint8_t* m_buffer;
	size_t m_buflen, m_total_write;
};

}  // namespace zling
}  // namespace baidu

size_t zling_compress(int level, unsigned char* buf, size_t len, unsigned char* outbuf, size_t outlen);
size_t zling_decompress(unsigned char* buf, size_t len, unsigned char* outbuf, size_t outlen);

#endif  // SRC_LIBZLING_UTILS_H
