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
	int listen_sock_;
	const std::function<void(std::unique_ptr<FastCGIRequest>)> callback_;
	const std::unordered_set<std::string_view> headers_;
};
