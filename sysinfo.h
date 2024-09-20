#pragma once

#include "nitro.h"
#include <sys/utsname.h>
#include <fstream>

struct SysInfo{
	struct utsname uts;
	std::string hostname;
	std::string serialnumber;
	long pageSize;
	long physicalPages;
	long availablePages;

	static std::string readSerialNumber(){
		std::string sn="0000-0000-00000";
		std::ifstream stream;
		stream.open("/sys/firmware/devicetree/base/serial-number");
		if(stream.good()){
			std::getline(stream,sn);
		}else{
			stream.open("/sys/class/net/eth0/address");
			if(stream.good()){
				std::getline(stream,sn);
			}
		}
		return sn;
	}

	SysInfo(){
		char chars[512];
		int error=gethostname(chars,512);
		if(error==0){
			hostname=std::string(chars);
		}
		error=uname(&uts);
		serialnumber=readSerialNumber();

		pageSize = sysconf(_SC_PAGE_SIZE);
		physicalPages = sysconf(_SC_PHYS_PAGES);
		availablePages = sysconf(_SC_AVPHYS_PAGES);
	}
	
	std::string domain(){
		return std::string(uts.domainname);
	}

	std::string machine(){	//x86_64
		return std::string(uts.machine);
	}

	int jsonifyInfo(std::string session,std::string token,std::string &result){
		char *user=getenv("USER");
		std::stringstream ss;
		ss<<"{";
		ss<<"\"Name\":\""<<uts.nodename<<"\",";
		ss<<"\"Domain\":\""<<uts.domainname<<"\",";
		ss<<"\"Machine\":\""<<uts.machine<<"\",";
		ss<<"\"System\":\""<<uts.sysname<<"\",";
	//	ss<<"\"version\":\""<<uts.version<<"\",";
		ss<<"\"Release\":\""<<uts.release<<"\",";
		ss<<"\"User\":\""<<user<<"\",";
		ss<<"\"Session\":\""<<session<<"\",";
		ss<<"\"Token\":\""<<token<<"\"";
		ss<<"}";
		result=ss.str();
		return 0;
	}

};

int jsonifySysInfo(std::string sessionName,std::string sessionToken,std::string &result){
	SysInfo sysinfo;
	int error=sysinfo.jsonifyInfo(sessionName,sessionToken,result);
	return error;
}
