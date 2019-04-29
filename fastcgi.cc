#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <thread>

#include "fastcgi.h"
#include "fastcgi_conn.h"

FastCGIServer::FastCGIServer(int port, const std::function<void(std::unique_ptr<FastCGIRequest>)>& callback)
		: callback_(callback) {
	LOG(INFO) << "listening on [::1]:" << port;

	signal(SIGPIPE, SIG_IGN);

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
}

void FastCGIServer::Serve() {
	while (true) {
		sockaddr_in6 client_addr;
		socklen_t client_addr_len = sizeof(client_addr);

		auto client_sock = accept(listen_sock_, (sockaddr*) &client_addr, &client_addr_len);
		PCHECK(client_sock >= 0) << "accept()";
		CHECK_EQ(client_addr.sin6_family, AF_INET6);

		auto *conn = new FastCGIConn(client_sock, client_addr, callback_);
		std::thread thread([conn]() { conn->Serve(); });
		thread.detach();
	}
}


