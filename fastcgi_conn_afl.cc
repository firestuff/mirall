#include "fastcgi_conn.h"

int main(int argc, char* argv[]) {
	setenv("GLOG_logtostderr", "1", true);
	google::InitGoogleLogging(argv[0]);

	{
		FastCGIConn conn(STDIN_FILENO, {}, [](std::unique_ptr<FastCGIRequest> req) { req->End(); }, {});
		static_cast<void>(conn.Read());
	}

	gflags::ShutDownCommandLineFlags();
	google::ShutdownGoogleLogging();

	return 0;
}
