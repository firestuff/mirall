#include "sse.h"

SSEServer::SSEServer(int port, const std::function<void(std::unique_ptr<SSEStream>)>& callback)
		: fastcgi_server_(port, [](std::unique_ptr<FastCGIRequest> request) {}) {}

void SSEServer::Serve() {
	fastcgi_server_.Serve();
}
