#pragma once

#include "fastcgi.h"
#include "sse_stream.h"

class SSEServer {
  public:
	SSEServer(int port, const std::function<void(std::unique_ptr<SSEStream>)>& callback);
	void Serve();

  private:
  	FastCGIServer fastcgi_server_;
};
