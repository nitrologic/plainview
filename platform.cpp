// platform.cpp

// (c) 2024 Simon Armstrong - All rights reserved

// socket code courtesy github.com/nitrologic/nitro/nitro/nitrosockets.cpp

//#define HAS_UDT

#include <stdio.h>
#include <iostream>
#include <string.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#include <profileapi.h>
#endif
#ifdef __APPLE__
#include <sys/time.h>
#endif

#include "platform.h"

uint64_t cpuTime() {
	uint64_t micros;
#ifdef WIN32
	LARGE_INTEGER frequency;
	LARGE_INTEGER count;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&count);
	micros = (count.QuadPart * (uint64_t)1e6) / frequency.QuadPart;
#endif
#ifdef __APPLE__
	timeval tv;
	gettimeofday(&tv, nullptr);
	micros = tv.tv_sec * 1000000ULL + tv.tv_usec;
#endif
	return micros;
}

float peekf(void *p);

void processOSC(char *packet, int length){
	int p = 0;
	const char *address;
	const char *format;

	while (p<length) {
		int code = packet[p];
		switch (code){

		case 47:{   // /address
			address = packet + p;
			printf("address %s\n", address);
			size_t n = strlen(address);
			int n4 = (n + 1 + 3)&-4;
			p += n4;
			break;
		}

		case 44:{	// ,format
			const char *format = packet + p;
			printf("format %s\n", format);
			size_t n = strlen(format);
			int n4 = (n + 1 + 3)&-4;
			p += n4;
			for (const char *f = format + 1; *f; f++){
				char typetag = *f;
				switch (typetag){
					//						case 'i':
					//							break;
				case 'f':{
					float f = peekf(packet + p);
					printf("F:%f\n", f);
					p += 4;
					break; }
					//						case 's':
					//						case 'b':
					//							break;
				default:
					printf("Unhandled OSC typetag %d %s\n", typetag, packet + p);
					return;
				}
			}

			break;
		}

		default:
			printf("Unhandled OSC code %d %s\n", code, packet + p);
			return;
			break;
		}
	}

}


// posix is below

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#ifdef HAS_UDT

#include <udt.h>

#endif


#include <objbase.h>

void initSystem() {
	CoInitialize(NULL);
}

bool haveWinsock = false;

void tidyUp() {
	WSACleanup();
}

Socket::Socket(int64_t descriptor) {
	fd = descriptor;
}

Socket::~Socket() {
	if (fd) {
		close();
	}
}

float peekf(void *p){
	const unsigned int i = ntohl(*((unsigned int *)p));
	return *((float *)(&i));
}

WSADATA wsaData;

bool initWinsock(){
	if (haveWinsock) return true;
	int result = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (result) {
		printf("WSAStartup failed with error: %d\n", result);
		return false;
	}	
	haveWinsock=true;
	return true;
}

int Socket::testOSCIn(){
	initWinsock();

	SOCKET s;
	sockaddr_in a;
	sockaddr_in b;
	int blen = sizeof(b);

	char buffer[549];

	u_short hostPort = 7001;

	s= socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s == INVALID_SOCKET){
		printf("socket fail\n");
		return -5;
	}
	a.sin_family = AF_INET;
	a.sin_port = htons(hostPort);
	a.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(s, (SOCKADDR*)&a, sizeof(a))!=0) {
		printf("%s\n", strerror(errno));
		return -3;
	}

	while (true){
		size_t count = recvfrom(s, buffer, sizeof(buffer), 0, (SOCKADDR*)&b, &blen);
		if (count == -1) {
			printf("%s\n", strerror(errno));
		}
		else if (count == sizeof(buffer)) {
			printf("datagram too large for buffer: truncated\n");
		}
		else {
			processOSC(buffer, count);
		}
	}
	closesocket(s);

}

int Socket::testOSCOut(){

	initWinsock();

//	const char *host = "255.255.255.255";

	const char *host = "192.168.1.255";
	//	const char *host="192.168.1.114";
	const char *port = "7000";

	//	char content[32];

	char content[] = { "/1/rotary1\0\0,f\0\0\0\0\0\0" };

	addrinfo hints = { 0 };
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = 0;
	hints.ai_flags = AI_ADDRCONFIG;

	addrinfo* res = 0;
	int err = getaddrinfo(host, port, &hints, &res);
	if (err != 0) {
		printf("failed to resolve remote socket address (err=%d)\n", err);
		return -1;
	}

	int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (fd == -1) {
		printf("%s\n", strerror(errno));
		return -2;
	}

	char broadcast[] = { 1, 1, 1, 1 };
//	int broadcast = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, broadcast, sizeof(broadcast)) == -1) {
		printf("%s\n", strerror(errno));
		return -3;
	}


	if (sendto(fd, content, sizeof(content), 0, res->ai_addr, res->ai_addrlen) == -1) {
		printf("%s\n", strerror(errno));
		return -3;
	}

	return 0;
}

Socket* Socket::connect(const char *host,int port){	
	if(!initWinsock()){
		return 0;
	}

	addrinfo hints = { 0 };
	hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

	addrinfo *address;

	char portname[6];
	char * p=_itoa(port&65535, portname, 10);

	int result = getaddrinfo(host, portname, &hints, &address);
	if ( result) {
		printf("getaddrinfo failed with error: %d\n", result);
		return 0;
	}

	SOCKET s = INVALID_SOCKET;

	for(addrinfo *ptr=address; ptr != NULL ;ptr=ptr->ai_next) {
		s = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);		
		if (s == INVALID_SOCKET) {
			printf("socket failed error: %ld\n", WSAGetLastError());
			continue;
		}	
		result = ::connect( s, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (result == SOCKET_ERROR) {
			closesocket(s);
			s = INVALID_SOCKET;
			continue;
		}
		break;
	}
	
	freeaddrinfo(address);
	
	if (s == INVALID_SOCKET) {
		std::cout << "Socket connect fail" << std::endl;
		return 0;
	}
	
	std::cout << "Socket connect fd = " << (int)s << std::endl;

	return new Socket(s);
}

int Socket::accept() {

	SOCKET listen = fd;

	SOCKET s = INVALID_SOCKET;

	s = ::accept(listen, NULL, NULL);

	if (s == INVALID_SOCKET) {
		std::cout << "Socket accept fail " << WSAGetLastError() << std::endl;
		closesocket(s);
		return 1;
	}

	return 0;
}

void Socket::listen(int port, int flags, void *user){	

	if (!initWinsock()){
		return;
	}

	SOCKET s = INVALID_SOCKET;

	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != NO_ERROR) {
		wprintf(L"WSAStartup() failed with error: %d\n", result);
		return;
	}

	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET) {
		wprintf(L"socket function failed with error: %ld\n", WSAGetLastError());
		return;
	}

	sockaddr_in service;
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = inet_addr("127.0.0.1");
	service.sin_port = htons(port);

	result = bind(s, (SOCKADDR *)& service, sizeof(service));
	if (result == SOCKET_ERROR) {
		wprintf(L"bind function failed with error %d\n", WSAGetLastError());
		result = closesocket(s);
		if (result == SOCKET_ERROR)
			wprintf(L"closesocket function failed with error %d\n", WSAGetLastError());
		return;
	}

	// https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-listen

	if (::listen(s, SOMAXCONN) == SOCKET_ERROR){
		wprintf(L"listen function failed with error: %d\n", WSAGetLastError());
		return;
	}

	std::cout << "listening on port " << (int)port << std::endl;

	fd = s;

#ifdef sockets_stay_closed
	result = closesocket(s);
	if (result == SOCKET_ERROR) {
		wprintf(L"closesocket function failed with error %d\n", WSAGetLastError());
		return;
	}
#endif
}


Socket* Socket::open(int port, int flags) {
//	return nullptr;

	if (!initWinsock()) {
		return nullptr;
	}

	SOCKET s = INVALID_SOCKET;

	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != NO_ERROR) {
		wprintf(L"WSAStartup() failed with error: %d\n", result);
		return nullptr;
	}

	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET) {
		wprintf(L"socket function failed with error: %ld\n", WSAGetLastError());
		return nullptr;
	}

	sockaddr_in service;
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = inet_addr("127.0.0.1");
	service.sin_port = htons(port);

	result = bind(s, (SOCKADDR*)&service, sizeof(service));
	if (result == SOCKET_ERROR) {
		wprintf(L"bind function failed with error %d\n", WSAGetLastError());
		result = closesocket(s);
		if (result == SOCKET_ERROR)
			wprintf(L"closesocket function failed with error %d\n", WSAGetLastError());
		return  nullptr;
	}

	return new Socket(s);
}

int Socket::serve(Connection* service) {

	SOCKET s = fd;

	std::cout << "serving" << std::endl;

	int listening = ::listen(s, SOMAXCONN);

	if ( listening == SOCKET_ERROR) {
		wprintf(L"listen function failed with error: %d\n", WSAGetLastError());
		return 0;
	}

	sockaddr_in from = { 0 };
	socklen_t len = sizeof(sockaddr_in);
	SOCKET s2 = ::accept(s, (sockaddr*) & from, &len);

	Socket* link = new Socket(s2);
	if (service) {
		std::string address(inet_ntoa(from.sin_addr));
		service->onConnect(link, address);
	}

	return 0;
}

int Socket::receive(char *buffer,int count){
	int result = ::recv(fd, buffer, count, 0);
	std::cout << "::recv " << result << std::endl;
	return result;
}

int Socket::send(const char *buffer,int count){	

	int result = ::send(fd,buffer,count,0);

	if (result == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		close();
		return -1;
	}

	printf("Bytes Sent: %ld\n", result);
	return result;
}

	
void Socket::close(){
	closesocket(fd);
	fd = 0;
}

#else

#include <sys/types.h>
#include <sys/socket.h>
//#include <sys/unistd.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/uio.h>

void initSystem(){
	
}

// on nonzero result input prints string version of errno

bool posixError(int result){
	if(result==0) return false;
	result=errno;
	const char *err=strerror(errno);
	printf("Socket::posix_error(%d) %s\n",errno,err);
	return true;
}

float peekf(void *p){
	// convert from big-endian (network btye order)
	const uint32_t i = ntohl(*((uint32_t *) p));
	return *((float *) (&i));
}


// thanks to http://www.microhowto.info/howto/listen_for_and_receive_udp_datagrams_in_c.html

int Socket::testOSCIn(){
	const char *host=0;
	const char *port="7000";

	addrinfo hints={0};
	hints.ai_family=AF_UNSPEC;
	hints.ai_socktype=SOCK_DGRAM;
	hints.ai_protocol=0;
	hints.ai_flags=AI_PASSIVE|AI_ADDRCONFIG;

	addrinfo* res=0;
	int err=getaddrinfo(host,port,&hints,&res);
	if (err!=0) {
		printf("failed to resolve local socket address (err=%d)\n",err);
		return -1;
	}


	int fd=socket(res->ai_family,res->ai_socktype,res->ai_protocol);
	if (fd==-1) {
		printf("%s\n",strerror(errno));
		return -2;
	}
	
	if (bind(fd,res->ai_addr,res->ai_addrlen)==-1) {
		printf("%s\n",strerror(errno));
		return -3;
	}
	
	freeaddrinfo(res);

	char buffer[549];
	
	struct sockaddr_storage src_addr;
	socklen_t src_addr_len=sizeof(src_addr);

	while(true){
		ssize_t count=recvfrom(fd,buffer,sizeof(buffer),0,(struct sockaddr*)&src_addr,&src_addr_len);
		if (count==-1) {
			printf("%s\n",strerror(errno));
		} else if (count==sizeof(buffer)) {
			printf("datagram too large for buffer: truncated\n");
		} else {
			processOSC(buffer,count);
		}
	}

}

int Socket::testOSCOut(){

	const char *host="192.168.1.255";
//	const char *host="192.168.1.114";
	const char *port="7000";
	
//	char content[32];
	
	char content[]={"/1/rotary1\0\0,f\0\0\0\0\0\0"};
	
	addrinfo hints={0};
	hints.ai_family=AF_UNSPEC;
	hints.ai_socktype=SOCK_DGRAM;
	hints.ai_protocol=0;
	hints.ai_flags=AI_ADDRCONFIG;
	
	addrinfo* res=0;
	int err=getaddrinfo(host,port,&hints,&res);
	if (err!=0) {
		printf("failed to resolve remote socket address (err=%d)\n",err);
		return -1;
	}
	
	int fd=socket(res->ai_family,res->ai_socktype,res->ai_protocol);
	if (fd==-1) {
		printf("%s\n",strerror(errno));
		return -2;
	}
	
	
	int broadcast=1;
	if (setsockopt(fd,SOL_SOCKET,SO_BROADCAST, &broadcast,sizeof(broadcast))==-1) {
		printf("%s\n",strerror(errno));
		return -3;
	}
	
	
	if (sendto(fd,content,sizeof(content),0, res->ai_addr,res->ai_addrlen)==-1) {
		printf("%s\n",strerror(errno));
		return -3;
	}

	return 0;
}


Socket* Socket::connect(const char *host,int port){

	int result;

	int domain=AF_INET;
	int type=SOCK_STREAM;
	int protocol=0;

	int fd = socket(domain, type, protocol);	
	printf("Socket::socket fd=%d\n",fd);

	hostent *server = gethostbyname(host);
	if (server == NULL) {
		printf("Socket::ERROR, no such host\n");
		return 0;
	}
	
	unsigned char *ip32=(unsigned char *)server->h_addr;	
	printf("gethostbyname %d.%d.%d.%d len=%d\n",
		ip32[0],ip32[1],ip32[2],ip32[3], 
		server->h_length);
	fflush(stdout);

	sockaddr_in address={0};
	address.sin_family=AF_INET;
	address.sin_port=htons(port);
	memcpy( &address.sin_addr.s_addr, ip32, server->h_length);

printf("connecting..\n");
	fflush(stdout);

	result=::connect(fd,(sockaddr *)&address,sizeof(sockaddr_in));

printf("connected result=%d\n",result);
	fflush(stdout);

	if (posixError(result)) return 0;

	printf("Socket::socket connected\n");		
	return new Socket(fd);
}

Socket *Socket::open(int port, int flags){
	int domain=AF_INET;
	int type=SOCK_STREAM;
	int protocol=0;
	int fd = socket(domain, type, protocol);	
	printf("Socket::socket fd=%d\n",fd);
	if(fd<0){
		return 0;
	}
	int reuse=1;
	int setoptResult=setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
	sockaddr_in address={0};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(port);
	// todo: make safe array access
	int result = bind(fd,(sockaddr *)&address,sizeof(sockaddr_in));
	if (posixError(result)) return 0;
	return new Socket(fd);
}

#include <arpa/inet.h>

int Socket::accept() {
	std::cout << "accept not supported - please use serve connection api" << std::endl;
	return 0;
}


void Socket::listen(int port, int flags, void* user){
	std::cout << "listen not supported - please use serve connection api" << std::endl;
}

// listen and serve new connections from service

int Socket::serve(Connection *service)
{
	int backlog=32;
	int result=::listen(fd,backlog);
	if (posixError(result)) return 0;
	
	bool listening=true;

	while(listening){

		printf("Socket::listening...\n");

		sockaddr_in from={0};
		socklen_t len=sizeof(sockaddr_in);
	
		int fd2;
		fd2 = ::accept(fd,(sockaddr *) &from,&len);

		if(fd2<0){
			printf("Socket::accept fd2=%d\n",fd2);
			listening=false;
			continue;
		}

		Socket *link=new Socket(fd2);
		if(service){
			std::string address(inet_ntoa(from.sin_addr));
			service->onConnect(link,address);
		}
	}
	return 0;
}

Socket::Socket(int64_t descriptor){
	fd=descriptor;
// make non blocking
//	int iMode=1;
//	ioctl(fd, FIONBIO, &iMode);		
}

Socket::~Socket(){
	if(fd){
		close();
	}
}

int Socket::receive(char *buffer,int count){
	ssize_t result = ::recv(fd, buffer, count, 0);	//MSG_DONTWAIT
	std::cout << "::recv " << result << std::endl;
	return (int)result;
}

int Socket::send(const char *buffer,int count){
	size_t result = ::send(fd, buffer, count, 0);	
	return (int)result;
}
	
void Socket::close(){
	::close(fd);
	fd=0;
}

#endif



#ifdef WIN32



#include <winhttp.h>

#include <codecvt>
#include <locale>
#include <utility>
#include <fstream>
#include <iomanip>


// utility wrapper to adapt locale-bound facets for wstring/wbuffer convert
template<class Facet>
struct deletable_facet : Facet
{
	template<class ...Args>
	deletable_facet(Args&& ...args) : Facet(std::forward<Args>(args)...) {}
	~deletable_facet() {}
};

std::wstring_convert<deletable_facet<std::codecvt<wchar_t, char, std::mbstate_t>>, wchar_t> conv16;

std::wstring WideFromUtf8(const char *data) {
	std::wstring result;
	std::wstring wstr = conv16.from_bytes(data);
	return wstr;
}

const int BufferSize = 8192 * 1024;
char pszOutBuffer[8192 * 1024];

void pingHost(const char* _userAgent, const char* _hostName, int hostPort) {

	DWORD flags = WINHTTP_FLAG_ESCAPE_PERCENT;
	DWORD dwSize = 0;
	DWORD dwDownloaded = 0;
	BOOL  bResults = FALSE;
	HINTERNET  hSession = NULL,
		hConnect = NULL,
		hRequest = NULL;

	const std::wstring userAgent = WideFromUtf8(_userAgent);
	const std::wstring hostName = WideFromUtf8(_hostName);
//	const wchar_t* hostName


	// Use WinHttpOpen to obtain a session handle.
//	hSession = WinHttpOpen(L"WinHTTP pingHost/1.0",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS, 0);

	hSession = WinHttpOpen(userAgent.c_str(), WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);

	// Specify an HTTP server.
	if (hSession)
		hConnect = WinHttpConnect(hSession, hostName.c_str(), hostPort, 0);

	const wchar_t* accept[] = { L"application/json",0 };//	WINHTTP_DEFAULT_ACCEPT_TYPES;
	// Create an HTTP request handle.
	if (hConnect)
		hRequest = WinHttpOpenRequest(hConnect, L"GET", NULL, NULL, WINHTTP_NO_REFERER, accept, flags);

	// Send a request.
	if (hRequest)
		bResults = WinHttpSendRequest(hRequest,
			WINHTTP_NO_ADDITIONAL_HEADERS, 0,
			WINHTTP_NO_REQUEST_DATA, 0,
			0, 0);


	// End the request.
	if (bResults)
		bResults = WinHttpReceiveResponse(hRequest, NULL);

	// Keep checking for data until there is nothing left.

	if (bResults)
	{
		do
		{
			dwSize = 0;
			BOOL ok = WinHttpQueryDataAvailable(hRequest, &dwSize);
			if (ok) {
				DWORD size = dwSize < BufferSize ? dwSize : BufferSize - 1;
				ok = WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, size, &dwDownloaded);
				if (ok) {
					pszOutBuffer[size] = 0;
					std::cout << pszOutBuffer << std::endl;
				}
				else {
					std::cout << "WinHttpReadData error " << GetLastError() << std::endl;
				}
			}
			else {
				std::cout << "WinHttpQueryDataAvailable error " << GetLastError() << std::endl;
			}
		} while (dwSize > 0);
	}
	// Report any errors.
	if (!bResults)
		std::cout << "WinHttpReceiveResponse error " << GetLastError() << std::endl;

	// Close any open handles.
	if (hRequest) WinHttpCloseHandle(hRequest);
	if (hConnect) WinHttpCloseHandle(hConnect);
	if (hSession) WinHttpCloseHandle(hSession);
}

#else

#include <curl/curl.h>


void pingHost(const char* userAgent, const char* hostName, int hostPort) {

  CURL *curl;
  CURLcode res;
 
  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, hostName);
    /* example.com is redirected, so we tell libcurl to follow redirection */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
 
    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
 
    /* always cleanup */
    curl_easy_cleanup(curl);
  }
}

#endif






struct SocketConnection :Device {

	Socket* _socket;
	std::string _address;

	SocketConnection(std::string address, Socket* socket) {
		_socket = socket;
		_address = address;
		Open("http:");
	}

	virtual void CloseDevice() {

	}
	virtual void onRead(Bytes payload) {

	}
	virtual size_t readBytes(void* buffer, size_t length) {
		int result = _socket->receive((char*)buffer, length);
		return (size_t)result;
	}
	virtual size_t writeBytes(const void* buffer, size_t length) {
		return 0;
	}

};

Device* openSocket(std::string from, Socket* s) {
	return new SocketConnection(from, s);
}
