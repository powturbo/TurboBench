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
#include "libzling_utils_mem.h"
#include "../libzling/src/libzling.h"

namespace baidu {
namespace zling {


size_t MemInputter::GetData(unsigned char* buf, size_t len) {
	if (len > m_buflen - m_total_read)
		len = m_buflen - m_total_read;

	memcpy(buf, m_buffer + m_total_read, len);
	m_total_read += len;
	return len;
}
bool MemInputter::IsEnd() {
	return m_total_read >= m_buflen;
}
bool MemInputter::IsErr() {
	return false;
}
size_t MemInputter::GetInputSize() {
    return m_total_read;
}

size_t MemOutputter::PutData(unsigned char* buf, size_t len) {
	if (len > m_buflen - m_total_write)
		len = m_buflen - m_total_write;

	memcpy(m_buffer + m_total_write, buf, len);
	m_total_write += len;
	return len;
}

bool MemOutputter::IsErr() {
	return m_total_write > m_buflen;
}
size_t MemOutputter::GetOutputSize() {
    return m_total_write;
}

}  // namespace zling
}  // namespace baidu

size_t zling_compress(int level, unsigned char* buf, size_t len, unsigned char* outbuf, size_t outlen)
{
	baidu::zling::MemInputter  inputter(buf, len);
	baidu::zling::MemOutputter outputter(outbuf, outlen);
	baidu::zling::Encode(&inputter, &outputter,NULL,level);

	return outputter.GetOutputSize();
}

size_t zling_decompress(unsigned char* buf, size_t len, unsigned char* outbuf, size_t outlen)
{
	baidu::zling::MemInputter  inputter(buf, len);
	baidu::zling::MemOutputter outputter(outbuf, outlen);
	baidu::zling::Decode(&inputter, &outputter);

	return outputter.GetOutputSize();
}
