#include <arpa/inet.h>
#include <netinet/in.h>

#include "fastcgi_conn.h"

namespace {

struct fcgi_header {
	uint8_t version;
	uint8_t type;
  private:
	uint16_t request_id_; // network byte order
	uint16_t content_length_; // network byte order
  public:
	uint8_t padding_length;
	uint8_t reserved;

	uint16_t ContentLength() const { return htons(content_length_); }
};

struct fcgi_begin_request {
	uint16_t role; // network byte order
	uint8_t flags;
	uint8_t reserved[5];
};

constexpr auto fcgi_max_content_len = 65535;
constexpr auto fcgi_max_padding_len = 255;
constexpr auto fcgi_max_record_len = sizeof(fcgi_header) + fcgi_max_content_len + fcgi_max_padding_len;

} // namespace

FastCGIConn::FastCGIConn(int sock, const sockaddr_in6& client_addr)
		: sock_(sock),
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
		}
		buf_.Wrote(read_len);

		ParseBuf();
		buf_.Consume(); // free buffer tail space for next read
	}
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

		LOG(INFO) << "type: " << (int)header->type;

		switch (header->type) {
		  case 1:
		  	{
				CHECK_EQ(header->ContentLength(), sizeof(fcgi_begin_request));
				const auto *begin_request = CHECK_NOTNULL(buf_.ReadObj<fcgi_begin_request>());
				CHECK_EQ(ntohs(begin_request->role), 1);
			}
			break;

		  case 4:
		    {
				const auto *params = buf_.Read(header->ContentLength());
			}
			break;
		}

		CHECK(buf_.Discard(header->padding_length));

		buf_.Commit(); // we've acted on the bytes read so far
	}
}
