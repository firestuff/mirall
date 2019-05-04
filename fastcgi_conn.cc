#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/uio.h>

#include "fastcgi_conn.h"

#include "fastcgi_parse.h"
#include "fastcgi_request.h"

FastCGIConn::FastCGIConn(int sock, const sockaddr_in6& client_addr, const std::function<void(std::unique_ptr<FastCGIRequest>)>& callback, const std::unordered_set<std::string_view>& headers)
		: sock_(sock),
		  callback_(callback),
		  headers_(headers),
		  buf_(sock, fastcgi_max_record_len) {
	char client_addr_str[INET6_ADDRSTRLEN];
	PCHECK(inet_ntop(AF_INET6, &client_addr.sin6_addr, client_addr_str, sizeof(client_addr_str)));

	LOG(INFO) << "new connection: [" << client_addr_str << "]:" << ntohs(client_addr.sin6_port);
}

FastCGIConn::~FastCGIConn() {
	PCHECK(close(sock_) == 0);
	LOG(INFO) << "connection closed (handled " << requests_ << " requests)";
}

bool FastCGIConn::Write(const std::vector<iovec>& vecs) {
	size_t total_size = 0;
	for (const auto& vec : vecs) {
		total_size += vec.iov_len;
	}
	return writev(sock_, vecs.data(), vecs.size()) == total_size;
}

bool FastCGIConn::Read() {
	if (!buf_.Refill()) {
		return false;
	}

	while (true) {
		buf_.ResetRead();

		const auto *header = buf_.ReadObj<FastCGIHeader>();
		if (!header) {
			break;
		}

		CHECK_EQ(header->version, 1);
		if (buf_.ReadMaxLen() < header->ContentLength()) {
			break;
		}

		switch (header->type) {
		  case 1:
		  	{
				CHECK_EQ(header->ContentLength(), sizeof(FastCGIBeginRequest));
				const auto *begin_request = CHECK_NOTNULL(buf_.ReadObj<FastCGIBeginRequest>());
				CHECK_EQ(begin_request->Role(), 1);

				request_.reset(new FastCGIRequest(header->RequestId(), this));
			}
			break;

		  case 4:
		    {
				CHECK_EQ(header->RequestId(), request_->RequestId());
				ConstBuffer param_buf(buf_.Read(header->ContentLength()), header->ContentLength());
				while (param_buf.ReadMaxLen() > 0) {
					const auto *param_header = param_buf.ReadObj<FastCGIParamHeader>();
					std::string_view key(param_buf.Read(param_header->key_length), param_header->key_length);
					std::string_view value(param_buf.Read(param_header->value_length), param_header->value_length);
					if (headers_.find(key) != headers_.end()) {
						request_->AddParam(key, value);
					}
				}
			}
			break;

		  case 5:
		  	{
				CHECK_EQ(header->RequestId(), request_->RequestId());
				if (header->ContentLength() == 0) {
					// Magic signal for completed request (mirrors the HTTP/1.1 protocol)
					requests_++;
					callback_(std::move(request_));
				} else {
					std::string_view in(buf_.Read(header->ContentLength()), header->ContentLength());
					request_->AddIn(in);
				}
			}
			break;

		  default:
			CHECK(false) << "unknown record type: " << header->type;
			break;
		}

		if (!buf_.Discard(header->padding_length)) {
			break;
		}

		buf_.Commit(); // we've acted on the bytes read so far
	}

	buf_.Consume();
	return true;
}

int FastCGIConn::Sock() {
	return sock_;
}
