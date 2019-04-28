#include <gflags/gflags.h>
#include <glog/logging.h>

#include "fastcgi.h"

DEFINE_int32(port, 9000, "TCP port to bind");

int main(int argc, char *argv[]) {
	google::InitGoogleLogging(argv[0]);
	gflags::ParseCommandLineFlags(&argc, &argv, true);

	FastCGIServer server(FLAGS_port);
	server.Serve();
}
