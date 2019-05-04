#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include "fastcgi.h"
#include "fastcgi_conn.h"

FastCGIServer::FastCGIServer(int port, const std::function<void(std::unique_ptr<FastCGIRequest>)>& callback, const std::unordered_set<std::string_view>& headers)
		: callback_(callback),
		  headers_(headers) {
	LOG(INFO) << "listening on [::1]:" << port;

	signal(SIGPIPE, SIG_IGN);

	epoll_fd_ = epoll_create1(0);
	PCHECK(epoll_fd_ >= 0) << "epoll_create()";

	listen_sock_ = socket(AF_INET6, SOCK_STREAM, 0);
	PCHECK(listen_sock_ >= 0) << "socket()";

	{
		int optval = 1;
		PCHECK(setsockopt(listen_sock_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) == 0);
	}

	{
		sockaddr_in6 bind_addr = {
			.sin6_family = AF_INET6,
			.sin6_port = htons(port),
			.sin6_addr = IN6ADDR_LOOPBACK_INIT,
		};
		PCHECK(bind(listen_sock_, (sockaddr*) &bind_addr, sizeof(bind_addr)) == 0);
	}

	PCHECK(listen(listen_sock_, 128) == 0);

	{
		struct epoll_event ev{
			.events = EPOLLIN,
			.data = {
				.ptr = nullptr,
			},
		};
		PCHECK(epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_sock_, &ev) == 0);
	}
}

void FastCGIServer::Serve() {
	while (true) {
		constexpr auto max_events = 256;
		struct epoll_event events[max_events];
		auto num_fd = epoll_wait(epoll_fd_, events, max_events, -1);
		if (num_fd == -1 && errno == EINTR) {
			continue;
		}
		PCHECK(num_fd > 0) << "epoll_wait()";

		for (auto i = 0; i < num_fd; ++i) {
			if (events[i].data.ptr == nullptr) {
				NewConn();
			} else {
				auto conn = static_cast<FastCGIConn*>(events[i].data.ptr);
				if (!conn->Read()) {
					PCHECK(epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, conn->Sock(), nullptr) == 0);
					delete conn;
				}
			}
		}
	}
}

void FastCGIServer::NewConn() {
	sockaddr_in6 client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	auto client_sock = accept(listen_sock_, (sockaddr*) &client_addr, &client_addr_len);
	PCHECK(client_sock >= 0) << "accept()";
	CHECK_EQ(client_addr.sin6_family, AF_INET6);

	{
		auto *conn = new FastCGIConn(client_sock, client_addr, callback_, headers_);
		struct epoll_event ev{
			.events = EPOLLIN,
			.data = {
				.ptr = conn,
			},
		};
		PCHECK(epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_sock, &ev) == 0);
	}
}


