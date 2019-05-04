#pragma once

#include "buffer.h"

class StreamBuffer : public Buffer {
  public:
	StreamBuffer(int sock, size_t size);

	[[nodiscard]] bool Refill();

  private:
  	int sock_;
};
