#pragma once

#include <unordered_map>

class FastCGIConn;

class FastCGIRequest {
  public:
    FastCGIRequest(uint16_t request_id, FastCGIConn *conn);

	void AddParam(const std::string_view& key, const std::string_view& value);
	void AddIn(const std::string_view& in);

	const std::string& GetParam(const std::string& key);

	void Write(const std::vector<std::pair<std::string_view, std::string_view>>& headers, const std::vector<std::string_view>& body);
	void WriteEnd();

  private:
	const uint16_t request_id_;
	FastCGIConn *conn_;

	std::unordered_map<std::string, std::string> params_;
	std::string in_;
	bool body_sent_ = false;
};
