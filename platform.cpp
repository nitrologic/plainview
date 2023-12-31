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

Socket::Socket(int descriptor) {
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


bool haveWinsock=false;
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

	u_short hostPort = 7000;

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

	if (::listen(s, SOMAXCONN) == SOCKET_ERROR){
		wprintf(L"listen function failed with error: %d\n", WSAGetLastError());
		return;
	}

	std::cout << "listening on port " << (int)port << std::endl;

#ifdef sockets_stay_closed
	result = closesocket(s);
	if (result == SOCKET_ERROR) {
		wprintf(L"closesocket function failed with error %d\n", WSAGetLastError());
		return;
	}
#endif
}



//Socket::Socket(int descriptor,void *userdata){
//	fd=descriptor;
//	user=userdata;
//}

const char *Socket::read(){
	buffer[0] = 0;

	int result = recv(fd, buffer, BufferSize, 0);
	if (result > 0){
		printf("Bytes received: %d\n", result);
		buffer[result] = 0;
	}else if (result == 0){
		printf("Connection closed\n");
		return 0;
	}else{
		printf("recv failed with error: %d\n", WSAGetLastError());
		close();
	}
	return buffer;
}

int Socket::write(const char *bytes,int count){	

	int result = ::send(fd,bytes,count,0);

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

int Socket::listen(Connection *service)
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
		fd2=accept(fd,(sockaddr *) &from,&len);

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
	
}

Socket::Socket(int descriptor){
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

const char *Socket::read(){
	buffer[0]=0;
	
	ssize_t count=::recv(fd,buffer,BufferSize-1,0);	//MSG_DONTWAIT
	if(count>0){
		buffer[count]=0;
//		printf("Socket::read count=%d\n",count);
	}else{
		if (count==0){
			printf("Socket::read is end of stream\n",count);				
			return 0;			
		}
		if (count==-1){
			printf("Socket::read has error\n",count);
			return 0;			
		}
	}
	return buffer;
}

int Socket::write(const char *bytes,int count){
	// todo: make me safe	
	if (count==0){
		count=strlen(bytes);
	}
	size_t result=::write(fd,bytes,count);	
//	printf("Socket::write count=%d result=%d\n",count,result);	
	return result;
}
	
void Socket::close(){
	::close(fd);
	fd=0;
}

#endif
