#include "sse.h"

SSEServer::SSEServer(int port, const std::function<void(std::unique_ptr<SSEStream>)>& callback)
		: callback_(callback),
		  fastcgi_server_(port, [this](std::unique_ptr<FastCGIRequest> request) { OnRequest(std::move(request)); }) {}

void SSEServer::Serve() {
	fastcgi_server_.Serve();
}

void SSEServer::OnRequest(std::unique_ptr<FastCGIRequest> request) {
	if (request->GetParam("HTTP_ACCEPT") != "text/event-stream") {
		LOG(WARNING) << "bad HTTP_ACCEPT: " << request->GetParam("HTTP_ACCEPT");
		request->WriteHeader("Status", "400 Bad Request");
		request->WriteHeader("Content-Type", "text-plain");
		request->WriteBody("No \"Accept: text/event-stream\" header found in request. Please call this endpoint using EventSource.");
		request->End();
		return;
	}
	request->WriteHeader("Content-Type", "text/event-stream");
	request->WriteBody("");
	callback_(std::make_unique<SSEStream>(std::move(request)));
}
