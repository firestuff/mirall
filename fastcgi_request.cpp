#include <sys/uio.h>

#include "fastcgi_conn.h"

#include "fastcgi_request.h"

FastCGIRequest::FastCGIRequest(uint16_t request_id, FastCGIConn* conn)
		: request_id_(request_id),
		  conn_(conn) {}

void FastCGIRequest::AddParam(const std::string_view& key, const std::string_view& value) {
	params_.try_emplace(std::string(key), std::string(value));
}

void FastCGIRequest::AddIn(const std::string_view& in) {
	in_.append(in);
}

const std::string& FastCGIRequest::GetParam(const std::string& key) {
	return params_.at(key);
}

void FastCGIRequest::Write(const std::vector<std::pair<std::string_view, std::string_view>>& headers, const std::vector<std::string_view>& body) {
	std::vector<iovec> vecs;
	vecs.reserve((headers.size() * 4) + 1 + body.size());

	CHECK(headers.empty() || !body_sent_);

	for (const auto& header : headers) {
		vecs.push_back(iovec{
			.iov_base = const_cast<char*>(header.first.data()),
			.iov_len = header.first.size(),
		});
		vecs.push_back(iovec{
			.iov_base = const_cast<char*>(": "),
			.iov_len = 2,
		});
		vecs.push_back(iovec{
			.iov_base = const_cast<char*>(header.second.data()),
			.iov_len = header.second.size(),
		});
		vecs.push_back(iovec{
			.iov_base = const_cast<char*>("\n"),
			.iov_len = 1,
		});
	}

	if (!body.empty() && !body_sent_) {
		body_sent_ = true;
		vecs.push_back(iovec{
			.iov_base = const_cast<char*>("\n"),
			.iov_len = 1,
		});
	}

	for (const auto& chunk : body) {
		vecs.push_back(iovec{
			.iov_base = const_cast<char*>(chunk.data()),
			.iov_len = chunk.size(),
		});
	}

	conn_->WriteOutput(request_id_, vecs);
}

void FastCGIRequest::WriteEnd() {
	conn_->WriteEnd(request_id_, 0);
}
