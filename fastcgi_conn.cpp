#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/uio.h>

#include "fastcgi_conn.h"

#include "fastcgi_parse.h"
#include "fastcgi_request.h"

FastCGIConn::FastCGIConn(int sock, const sockaddr_in6& client_addr, const std::function<void(std::unique_ptr<FastCGIRequest>)>& callback)
		: sock_(sock),
		  callback_(callback),
		  buf_(sock, fastcgi_max_record_len) {
	char client_addr_str[INET6_ADDRSTRLEN];
	PCHECK(inet_ntop(AF_INET6, &client_addr.sin6_addr, client_addr_str, sizeof(client_addr_str)));

	LOG(INFO) << "new connection: [" << client_addr_str << "]:" << ntohs(client_addr.sin6_port);
}

FastCGIConn::~FastCGIConn() {
	PCHECK(close(sock_) == 0);
	LOG(INFO) << "connection closed";
}

void FastCGIConn::WriteBlock(uint8_t type, uint16_t request_id, const std::vector<iovec>& vecs) {
	std::vector<iovec> out_vecs;
	out_vecs.reserve(vecs.size() + 1);

	FastCGIHeader header;
	header.version = 1;
	header.type = type;
	header.SetRequestId(request_id);
	uint16_t content_length = 0;
	for (auto& vec : vecs) {
		content_length += vec.iov_len;
	}
	header.SetContentLength(content_length);
	out_vecs.push_back(iovec{
		.iov_base = &header,
		.iov_len = sizeof(header),
	});

	for (auto& vec : vecs) {
		out_vecs.push_back(vec);
	}

	CHECK_EQ(writev(sock_, out_vecs.data(), out_vecs.size()), content_length + sizeof(header));
}

void FastCGIConn::WriteOutput(uint16_t request_id, const std::vector<iovec>& vecs) {
	WriteBlock(6, request_id, vecs);
}

void FastCGIConn::WriteEnd(uint16_t request_id) {
	FastCGIEndRequest end;

	std::vector<iovec> vecs;
	vecs.push_back(iovec{
		.iov_base = &end,
		.iov_len = sizeof(end),
	});
	WriteBlock(3, request_id, vecs);
}

void FastCGIConn::Serve() {
	while (true) {
		const auto *header = buf_.ReadObj<FastCGIHeader>();
		if (!header) {
			LOG(INFO) << "readobj failed";
			break;
		}

		CHECK_EQ(header->version, 1);
		if (!buf_.BufferAtLeast(header->ContentLength())) {
			return;
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
				ConstBuffer param_buf(buf_.Read(header->ContentLength()), header->ContentLength());
				while (param_buf.ReadMaxLen() > 0) {
					const auto *param_header = param_buf.ReadObj<FastCGIParamHeader>();
					std::string_view key(param_buf.Read(param_header->key_length), param_header->key_length);
					std::string_view value(param_buf.Read(param_header->value_length), param_header->value_length);
					request_->AddParam(key, value);
				}
			}
			break;

		  case 5:
		  	{
				if (header->ContentLength() == 0) {
					// Magic signal for completed request (mirrors the HTTP/1.1 protocol)
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

	delete this;
}
