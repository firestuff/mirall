#pragma once

#include <glog/logging.h>
#include <memory>

class ConstBuffer {
  public:
	ConstBuffer(const char *buf, size_t len);

	[[nodiscard]] size_t ReadMaxLen() const;
	[[nodiscard]] virtual const char *Read(size_t len);
	template<class T> [[nodiscard]] const T *ReadObj();

	bool Discard(size_t len); // like Read() but don't use the result

  protected:
	const char *const_buf_;
	size_t len_;
	size_t start_ = 0;
};

class Buffer : public ConstBuffer {
  public:
	Buffer(char *buf, size_t size, size_t len);
  	Buffer(size_t size);

	[[nodiscard]] char *WritePtr();
	[[nodiscard]] size_t WriteMaxLen() const;
	void Wrote(size_t len);

	void Commit(); // commit read position

  protected:
  	std::unique_ptr<char> own_buf_;
	char *buf_;
	const size_t size_;
};

template<class T> const T *ConstBuffer::ReadObj() {
	if (ReadMaxLen() < sizeof(T)) {
		return nullptr;
	}
	return reinterpret_cast<const T*>(Read(sizeof(T)));
}
