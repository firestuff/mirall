#pragma once

#include "buffer.h"

class StreamBuffer : public Buffer {
  public:
	StreamBuffer(int sock, size_t size);

	[[nodiscard]] bool BufferAtLeast(size_t len);
	[[nodiscard]] const char *Read(size_t len) override;
	template<class T> [[nodiscard]] const T *ReadObj();

  private:
  	int sock_;
};

template<class T> const T *StreamBuffer::ReadObj() {
	return reinterpret_cast<const T*>(Read(sizeof(T)));
}
