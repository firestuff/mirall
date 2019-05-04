#pragma once

#include <functional>
#include <memory>
#include <unordered_set>

#include "fastcgi_request.h"

class FastCGIServer {
  public:
	FastCGIServer(int port, const std::function<void(std::unique_ptr<FastCGIRequest>)>& callback, const std::unordered_set<std::string_view>& headers={});
	void Serve();

  private:
	void NewConn();

	const std::function<void(std::unique_ptr<FastCGIRequest>)> callback_;
	const std::unordered_set<std::string_view> headers_;
	int epoll_fd_;
	int listen_sock_;
};
