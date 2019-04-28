#pragma once

#include <functional>
#include <unordered_map>

#include "stream_buffer.h"

struct sockaddr_in6;
class FastCGIRequest;

class FastCGIConn {
  public:
	FastCGIConn(int sock, const sockaddr_in6& client_addr, const std::function<void(std::unique_ptr<FastCGIRequest>)>& callback);
	~FastCGIConn();

	void Serve();

	void Write(const std::vector<iovec>& vecs);

  private:
  	const int sock_;
  	std::function<void(std::unique_ptr<FastCGIRequest>)> callback_;

	uint64_t requests_ = 0;

	StreamBuffer buf_;

	std::unique_ptr<FastCGIRequest> request_;
};
