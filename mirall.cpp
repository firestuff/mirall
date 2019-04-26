#include <arpa/inet.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <netinet/in.h>
#include <sys/socket.h>

DEFINE_int32(port, 9000, "TCP port to bind");

int main(int argc, char *argv[]) {
	google::InitGoogleLogging(argv[0]);
	gflags::ParseCommandLineFlags(&argc, &argv, true);

	auto listen_sock = socket(AF_INET6, SOCK_STREAM, 0);
	PCHECK(listen_sock >= 0) << "socket()";

	sockaddr_in6 bind_addr = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(FLAGS_port),
		.sin6_addr = IN6ADDR_LOOPBACK_INIT,
	};
	PCHECK(bind(listen_sock, (sockaddr*) &bind_addr, sizeof(bind_addr)) == 0);

	PCHECK(listen(listen_sock, 128) == 0);

	while (true) {
		sockaddr_in6 client_addr;
		socklen_t client_addr_len = sizeof(client_addr);

		auto sock = accept(listen_sock, (sockaddr*) &client_addr, &client_addr_len);
		PCHECK(sock >= 0) << "accept()";

		CHECK_EQ(client_addr.sin6_family, AF_INET6);

		char client_addr_str[INET6_ADDRSTRLEN];
		PCHECK(inet_ntop(AF_INET6, &client_addr.sin6_addr, client_addr_str, sizeof(client_addr_str)));

		LOG(INFO) << "new connection: [" << client_addr_str << "]:" << ntohs(client_addr.sin6_port);
	}
}
