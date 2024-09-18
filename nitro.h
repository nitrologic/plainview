#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <sstream>
#include <iostream>

#include <iomanip>
#include <iterator>

using utf8 = std::string;
using utf32 = std::wstring;

typedef std::string escaped;
typedef std::vector<utf8> Strings;
typedef std::vector<uint8_t> Packet;
typedef std::pair<utf8,escaped> Attribute;
typedef std::map<utf8,escaped> Attributes;
typedef std::vector<Attributes> Data;

utf8 joinStrings(const Strings &strings, utf8 separator){
	std::stringstream ss;
	for(size_t i=0;i<strings.size();i++){
		if(i>0) ss << separator;
		ss<<strings[i];
	}
	return ss.str();
}

utf8 escape(const char *s,bool quoted=false){
	std::ostringstream o;
	if(quoted) o<<"\"";
	while(char c=*s++){
		switch (c) {
			case '"': o << "\\\""; break;
			case '\\': o << "\\\\"; break;
			case '\b': o << "\\b"; break;
			case '\f': o << "\\f"; break;
			case '\n': o << "\\n"; break;
			case '\r': o << "\\r"; break;
			case '\t': o << "\\t"; break;
			default:
				if ('\x00' <= c && c <= '\x1f') {
					o << "\\u"
					<< std::hex << std::setw(4) << std::setfill('0') << (int)c;
				} else {
					o << c;
				}
		}
	}
	if(quoted) o<<"\"";
	return o.str();
}

utf8 unquote(utf8 s){
	size_t n=s.size();
	return s.substr(1,n-2);
}

utf8 quoted(utf8 s){
	return escape(s.c_str(),true);
}

utf8 quotedString(const char *s){
	return(s==NULL)?"null":utf8("\"")+s+"\"";
}

utf8 nameValue(utf8 name,utf8 value){
	std::stringstream ss;
	ss<<quoted(name)<<":"<<quoted(value);
	return ss.str();
}

utf8 nameValue(const char *name,int value){
	std::stringstream ss;
	ss<<"{"<<quotedString(name)<<":"<<value<<"}";
	return ss.str();
}

utf8 utf8String(const char *s){
	return(s==NULL)?utf8(""):utf8(s);
}


Strings splitString(utf8 line,char delim){
	Strings split;
	std::stringstream ss(line);
	while(true){
		utf8 line;
		std::getline(ss,line,delim);
		if(line.length()==0) break;
		split.push_back(line);
	}
	return split;
}
