#pragma once

#include <cstdint>

void initSystem();

uint64_t cpuTime();

void pingHost(const char* userAgent, const char* hostName, int hostPort);

struct Socket{
	int64_t fd;

	static Socket* connect(const char* host, int port);

	static Socket* open(int port, int flags);

	Socket(int64_t descriptor);

	virtual ~Socket();
	
//	const char *read();

	int receive(char *, int n);

	int send(const char *,int n);

	int serve(struct Connection *service);

	void listen(int port, int flags, void* user);

	int accept();


//	int listen(struct Connection *service);

	void close();


	static int testOSCIn();
	static int testOSCOut();
};



struct Connection {
	virtual void onConnect(Socket* link, std::string address) = 0;
};

#include <cstdint>
#include <vector>
#include <thread>

using Bytes = std::vector<uint8_t>;

struct Device {

	std::string deviceName;

	std::thread* readThread;

	Bytes readBuffer;

	uint64_t deviceEpoch = 0;

	virtual void CloseDevice() = 0;
	virtual void onRead(Bytes payload) = 0;

	virtual size_t readBytes(void* buffer, size_t length) = 0;
	virtual size_t writeBytes(const void* buffer, size_t length) = 0;

	uint64_t deviceTime() {
		return cpuTime() - deviceEpoch;
	}

	void Open(std::string name) {
		deviceEpoch = cpuTime();
		deviceName = name;
		readBuffer.resize(8192);
		readThread = new std::thread(&Device::runRead, this);
	}

	void runRead() {
		while (true) {
			uint8_t* buffer = readBuffer.data();
			size_t count = readBuffer.size();
			if (count == 0) break;	// TODO : device not open device_error
			size_t n = readBytes(buffer, count);
			if (n == 0) break;
#ifdef LOG_DEVICE_READ			
			for (int i = 0; i < n; i++) {
				std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)buffer[i] << " ";
			}
			std::cout << std::dec << std::endl;
#endif		
			//#else
			//			std::cout << "*" << n << "*" << std::flush;
			onRead({ buffer, buffer + n });
		}
	}
};

Device *openSocket(std::string address, Socket *s);
