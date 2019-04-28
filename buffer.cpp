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
	Read(len);
	return true;
}

void ConstBuffer::ResetRead() {
	start_ = commit_;
}

void ConstBuffer::Commit() {
	commit_ = start_;
}


Buffer::Buffer(char *buf, size_t size, size_t len)
		: ConstBuffer(buf, size),
		  buf_(buf),
		  size_(size) {
	len_ = len;
}

Buffer::Buffer(size_t size)
		: Buffer((own_buf_.reset(new char[size]), own_buf_.get()), size, 0) {}

char *Buffer::WritePtr() {
	return &buf_[len_];
}

size_t Buffer::WriteMaxLen() const {
	return size_ - len_;
}

void Buffer::Wrote(size_t len) {
	CHECK_LE(len, WriteMaxLen());
	len_ += len;
}

void Buffer::Consume() {
	if (commit_ == 0) {
		return;
	}
	memmove(buf_, &buf_[commit_], len_ - commit_);
	len_ -= commit_;
	start_ -= commit_;
	commit_ = 0;
}
