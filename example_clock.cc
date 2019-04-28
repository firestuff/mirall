#include <gflags/gflags.h>
#include <glog/logging.h>

#include "sse.h"

DEFINE_int32(port, 9000, "TCP port to bind");

int main(int argc, char *argv[]) {
	google::InitGoogleLogging(argv[0]);
	gflags::ParseCommandLineFlags(&argc, &argv, true);

	SSEServer server(FLAGS_port, [](std::unique_ptr<SSEStream> stream) {
	});
	server.Serve();
}
