#pragma once

#include <functional>
#include <unordered_map>
#include <unordered_set>

#include "stream_buffer.h"

struct sockaddr_in6;
class FastCGIRequest;

class FastCGIConn {
  public:
	FastCGIConn(int sock, const sockaddr_in6& client_addr, const std::function<void(std::unique_ptr<FastCGIRequest>)>& callback, const std::unordered_set<std::string_view>& headers);
	~FastCGIConn();

	[[nodiscard]] bool Read();
	[[nodiscard]] bool Write(const std::vector<iovec>& vecs);

	[[nodiscard]] int Sock();

  private:
  	const int sock_;
	const std::function<void(std::unique_ptr<FastCGIRequest>)>& callback_;
	const std::unordered_set<std::string_view>& headers_;

	uint64_t requests_ = 0;

	StreamBuffer buf_;

	std::unique_ptr<FastCGIRequest> request_;
};
