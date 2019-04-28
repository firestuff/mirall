#include "stream_buffer.h"

StreamBuffer::StreamBuffer(int sock, size_t size)
		: Buffer(size),
		  sock_(sock) {}

bool StreamBuffer::BufferAtLeast(size_t len) {
	CHECK_LE(start_ + len, size_);

	while (ReadMaxLen() < len) {
		auto read_len = read(sock_, WritePtr(), WriteMaxLen());
		PCHECK(read_len >= 0);
		if (read_len == 0) {
			return false;
		}
		Wrote(read_len);
	}
	return true;
}

const char *StreamBuffer::Read(size_t len) {
	if (!BufferAtLeast(len)) {
		return nullptr;
	}
	return Buffer::Read(len);
}
