#pragma once

#include <functional>
#include <unordered_map>

#include "buffer.h"

struct sockaddr_in6;
class FastCGIRequest;

class FastCGIConn {
  public:
	FastCGIConn(int sock, const sockaddr_in6& client_addr, const std::function<void(std::unique_ptr<FastCGIRequest>)>& callback);
	~FastCGIConn();

	void Serve();

	void WriteBlock(uint8_t type, uint16_t request_id, const std::vector<iovec>& vecs);
	void WriteOutput(uint16_t request_id, const std::vector<iovec>& vecs);
	void WriteEnd(uint16_t request_id);

  private:
  	void ParseBuf();

  	const int sock_;
  	std::function<void(std::unique_ptr<FastCGIRequest>)> callback_;

	Buffer buf_;

	std::unique_ptr<FastCGIRequest> request_;
};
