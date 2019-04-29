#include <gflags/gflags.h>
#include <glog/logging.h>
#include <sys/time.h>

#include "sse.h"

DEFINE_int32(port, 9000, "TCP port to bind");

int main(int argc, char *argv[]) {
	google::InitGoogleLogging(argv[0]);
	gflags::ParseCommandLineFlags(&argc, &argv, true);

	SSEServer server(FLAGS_port, [](std::unique_ptr<SSEStream> stream) {
		while (true) {
			timeval tv;
			PCHECK(gettimeofday(&tv, nullptr) == 0);
			uint64_t time_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
			if (!stream->WriteEvent(std::to_string(time_ms), 0, "time")) {
				break;
			}
			sleep(1);
		}
	});
	server.Serve();
}
