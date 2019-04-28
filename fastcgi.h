#pragma once

#include <functional>
#include <memory>

#include "fastcgi_request.h"

class FastCGIServer {
  public:
  	FastCGIServer(int port, const std::function<void(std::unique_ptr<FastCGIRequest>)>& callback);
	void Serve();

  private:
	int listen_sock_;
  	std::function<void(std::unique_ptr<FastCGIRequest>)> callback_;
};
