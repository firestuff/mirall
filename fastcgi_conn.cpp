#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/uio.h>

#include "fastcgi_conn.h"

#include "fastcgi_request.h"

namespace {

struct fcgi_header {
	uint8_t version;
	uint8_t type;
  private:
	uint16_t request_id_; // network byte order
	uint16_t content_length_; // network byte order
  public:
	uint8_t padding_length = 0;
	uint8_t reserved = 0;

	uint16_t RequestId() const { return ntohs(request_id_); }
	uint16_t ContentLength() const { return ntohs(content_length_); }

	void SetRequestId(uint16_t request_id) { request_id_ = htons(request_id); }
	void SetContentLength(uint16_t content_length) { content_length_ = htons(content_length); }
};

struct fcgi_begin_request {
  private:
	uint16_t role_; // network byte order
  public:
	uint8_t flags;
	uint8_t reserved[5];

	uint16_t Role() const { return ntohs(role_); }
};

struct fcgi_end_request {
  private:
	uint32_t app_status_;
  public:
	uint8_t protocol_status;
	uint8_t reserved[3] = {};

	void SetAppStatus(uint32_t app_status) { app_status_ = htonl(app_status); }
};

struct fcgi_param_header {
	uint8_t key_length;
	uint8_t value_length;
};

constexpr auto fcgi_max_content_len = 65535;
constexpr auto fcgi_max_padding_len = 255;
constexpr auto fcgi_max_record_len = sizeof(fcgi_header) + fcgi_max_content_len + fcgi_max_padding_len;

} // namespace

FastCGIConn::FastCGIConn(int sock, const sockaddr_in6& client_addr, const std::function<void(std::unique_ptr<FastCGIRequest>)>& callback)
		: sock_(sock),
		  callback_(callback),
		  buf_(fcgi_max_record_len) {
	char client_addr_str[INET6_ADDRSTRLEN];
	PCHECK(inet_ntop(AF_INET6, &client_addr.sin6_addr, client_addr_str, sizeof(client_addr_str)));

	LOG(INFO) << "new connection: [" << client_addr_str << "]:" << ntohs(client_addr.sin6_port);
}

FastCGIConn::~FastCGIConn() {
	PCHECK(close(sock_) == 0);
	LOG(INFO) << "connection closed";
}

void FastCGIConn::Serve() {
	while (true) {
		auto read_len = read(sock_, buf_.WritePtr(), buf_.WriteMaxLen());
		PCHECK(read_len >= 0);
		if (read_len == 0) {
			LOG(INFO) << "peer closed connection";
			delete this;
			return;
		}
		buf_.Wrote(read_len);

		ParseBuf();
		buf_.Consume(); // free buffer tail space for next read
	}
}

void FastCGIConn::WriteBlock(uint8_t type, uint16_t request_id, const std::vector<iovec>& vecs) {
	std::vector<iovec> out_vecs;
	out_vecs.reserve(vecs.size() + 1);

	fcgi_header header;
	header.version = 1;
	header.type = type;
	header.SetRequestId(request_id);
	uint16_t content_length = 0;
	for (auto& vec : vecs) {
		content_length += vec.iov_len;
	}
	header.SetContentLength(content_length);
	out_vecs.push_back(std::move(iovec{
		.iov_base = &header,
		.iov_len = sizeof(header),
	}));

	for (auto& vec : vecs) {
		out_vecs.push_back(vec);
	}

	CHECK_EQ(writev(sock_, out_vecs.data(), out_vecs.size()), content_length + sizeof(header));
}

void FastCGIConn::WriteOutput(uint16_t request_id, const std::vector<iovec>& vecs) {
	WriteBlock(6, request_id, vecs);
}

void FastCGIConn::WriteEnd(uint16_t request_id, uint8_t status) {
	fcgi_end_request end;
	end.SetAppStatus(status);

	std::vector<iovec> vecs;
	vecs.push_back(std::move(iovec{
		.iov_base = &end,
		.iov_len = sizeof(end),
	}));
	WriteBlock(3, request_id, vecs);
}

void FastCGIConn::ParseBuf() {
	buf_.ResetRead();

	while (true) {
		const auto *header = buf_.ReadObj<fcgi_header>();
		if (!header) {
			return;
		}

		CHECK_EQ(header->version, 1);
		if (buf_.ReadMaxLen() < header->ContentLength() + header->padding_length) {
			return;
		}

		switch (header->type) {
		  case 1:
		  	{
				CHECK_EQ(header->ContentLength(), sizeof(fcgi_begin_request));
				const auto *begin_request = CHECK_NOTNULL(buf_.ReadObj<fcgi_begin_request>());
				CHECK_EQ(begin_request->Role(), 1);

				request_.reset(new FastCGIRequest(header->RequestId(), this));
			}
			break;

		  case 4:
		    {
				ConstBuffer param_buf(buf_.Read(header->ContentLength()), header->ContentLength());
				while (param_buf.ReadMaxLen() > 0) {
					const auto *param_header = param_buf.ReadObj<fcgi_param_header>();
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
		}

		CHECK(buf_.Discard(header->padding_length));

		buf_.Commit(); // we've acted on the bytes read so far
	}
}
