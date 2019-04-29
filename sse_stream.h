#pragma once

#include <memory>

class FastCGIRequest;

class SSEStream {
  public:
	SSEStream(std::unique_ptr<FastCGIRequest> request);

	[[nodiscard]] bool WriteEvent(const std::string& data, uint64_t id=0, const std::string& type="");
	bool End();

  private:
	std::unique_ptr<FastCGIRequest> request_;
};
