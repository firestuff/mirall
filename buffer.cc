#include "buffer.h"

ConstBuffer::ConstBuffer(const char *buf, size_t len)
		: const_buf_(buf),
		  len_(len) {}

size_t ConstBuffer::ReadMaxLen() const {
	return len_ - start_;
}

const char *ConstBuffer::Read(size_t len) {
	CHECK_LE(len, ReadMaxLen());
	const auto *ret = &const_buf_[start_];
	start_ += len;
	return ret;
}

bool ConstBuffer::Discard(size_t len) {
	if (len > ReadMaxLen()) {
		return false;
	}
	static_cast<void>(Read(len));
	return true;
}


Buffer::Buffer(char *buf, size_t size, size_t len)
		: ConstBuffer(buf, size),
		  buf_(buf),
		  size_(size) {
	len_ = len;
}

Buffer::Buffer(size_t size)
		: Buffer(new char[size], size, 0) {
	own_buf_.reset(buf_);
}

char *Buffer::WritePtr() {
	return &buf_[len_];
}

size_t Buffer::WriteMaxLen() const {
	return size_ - len_;
}

bool Buffer::Write(const std::string_view& str) {
	if (WriteMaxLen() < str.size()) {
		return false;
	}
	memcpy(WritePtr(), str.data(), str.size());
	Wrote(str.size());
	return true;
}

void Buffer::Wrote(size_t len) {
	CHECK_LE(len, WriteMaxLen());
	len_ += len;
}

void Buffer::Commit() {
	if (start_ == 0) {
		return;
	}
	memmove(buf_, &buf_[start_], len_ - start_);
	len_ -= start_;
	start_ = 0;
}
