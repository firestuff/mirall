#pragma once

class FastCGIServer {
  public:
  	FastCGIServer(int port);
	void Serve();

  private:
	int listen_sock_;
};
