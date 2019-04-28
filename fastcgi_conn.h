#pragma once

#include "buffer.h"

struct sockaddr_in6;

class FastCGIConn {
  public:
	FastCGIConn(int sock, const sockaddr_in6& client_addr);
	~FastCGIConn();

	void Serve();

  private:
  	void ParseBuf();

  	const int sock_;
	Buffer buf_;
};
