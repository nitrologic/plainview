#pragma once

uint64_t cpuTime();

struct Socket{
	static const int BufferSize=0x10000+1;	// 64K fragment setting for IP with post read null terminator
	char buffer[BufferSize];
	int fd;

	static Socket* connect(const char* host, int port);

	static Socket* open(int port, int flags);

	Socket(int descriptor);
	virtual ~Socket();
	
	const char *read();
	int write(const char *,int n);

	int serve(struct Connection *service);

	void listen(int port, int flags, void* user);

//	int listen(struct Connection *service);

	void close();


	static int testOSCIn();
	static int testOSCOut();
};



struct Connection {
	virtual void onConnect(Socket* link, std::string address) = 0;
};
