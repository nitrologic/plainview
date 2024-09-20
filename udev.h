#pragma once

#include "nitro.h"

#include <set>
#include <libudev.h>
#include <linux/serial.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <linux/joystick.h>

#define KB_STUFF

struct channel{
	int inputs;
	int outputs;
	std::string name;
	int bits[32];
	channel(){
	}
	channel(const char *n,int ins,int outs){		
		name=std::string(n);
		inputs=ins;
		outputs=outs;
		reset();
	}
	void reset(){
		std::fill(bits,bits+32,0);
	}
	std::string jsonify(){
		std::stringstream ss;
		ss<<"{\"name\":\""<<name<<"\",\"ins\":"<<inputs<<",\"outs\":"<<outputs<<"}";
		return ss.str();
	}
};

typedef std::map<std::string,channel> namedChannels;

std::string emptyString;

std::string stdString(const char *text){
	return text?std::string(text):emptyString;
}

struct deviceInfo{
	int inputs,outputs;
	std::string sysPath;
	std::string devPath;
	std::string sysName,devNode,driver;
	Attributes values;
	Attributes attributes;
	namedChannels channels;

	void dump(){
		std::cout << jsonify() << std::endl;
	}

	std::string jsonify(){
		Strings v,a,c;
		for(const auto &pair:values){
			v.push_back(nameValue(pair.first,pair.second));
		}
		for(const auto &pair:attributes){
			a.push_back(nameValue(pair.first,pair.second));
		}
		for(auto pair:channels){
			c.push_back(quoted(pair.first)+":"+pair.second.jsonify());
		}
		std::stringstream ss;
		ss<<"{\"values\":{";
		ss<<joinStrings(v,",");
		ss<<"},\"attributes\":{";
		ss<<joinStrings(a,",");
		ss<<"},\"channels\":{";
		ss<<joinStrings(c,",");
		ss<<"}}";
		return ss.str();
	}

	deviceInfo(const char *fpath):sysPath(fpath){
	}

	bool valueEquals(std::string name, std::string value){
		auto item=values.find(name);
		return item!=values.end() && item->second==value;
	}

	void addChannel(const char *name,int inputs,int outputs){
		channels.emplace(name,channel(name,inputs,outputs));
	}

	int scan(struct udev_device *in){
		const char *indent="\t";
		struct udev_list_entry *entry;
		entry=udev_device_get_properties_list_entry(in);
		while(entry){
			const char *name=udev_list_entry_get_name(entry);
			const char *value=udev_list_entry_get_value(entry);
			values[name]=stdString(value);
//			std::cout << indent << name << '=' << value << std::endl;
			entry=udev_list_entry_get_next(entry);
		}
		entry=udev_device_get_sysattr_list_entry(in);
//		entry=udev_device_get_devlinks_list_entry(in);
		while(entry){
			const char *name=udev_list_entry_get_name(entry);
			const char *value=udev_list_entry_get_value(entry);
			attributes[name]=stdString(value);
			entry=udev_list_entry_get_next(entry);
		}
		devPath=values["DEVNAME"];
		sysName=stdString(udev_device_get_sysname(in));
		devNode=stdString(udev_device_get_devnode(in));
		driver=stdString(udev_device_get_driver(in));
		return 0;
	}
};

static void udevList(struct udev_list_entry *entry ){
	while(entry){
		const char *indent="\t";
		const char *name=udev_list_entry_get_name(entry);
		const char *value=udev_list_entry_get_value(entry);
		std::cout << indent << name << '=' << value << std::endl;
		entry=udev_list_entry_get_next(entry);
	}
}


typedef std::set<deviceInfo*> deviceList;

struct Devices{
	deviceList usbList;
	deviceList inputList;
	deviceList outputList;

	Devices(){
	}

	int jsonifyDevices(Strings attributeMask,std::string& result){
		int error=enumInputs("input");
		ErrorResult(error,"jsonifyDevices enumerating Input s")
		Strings inputs;
		for(auto in:inputList){
			inputs.push_back(in->jsonify());
		}
		std::stringstream ss;
		ss<<"["<<joinStrings(inputs,",")<<"]";
		result=ss.str();
		std::cout << "jsonifyDevices: " << result << std::endl;
		return 0;
	}

	static deviceList udevCat(struct udev *udev,const char *subsystem,const char *devtype=0){
		deviceList cat;
		struct udev_enumerate *enumerate = udev_enumerate_new(udev);
		if(subsystem){
			udev_enumerate_add_match_subsystem(enumerate, subsystem);
		}
		if(devtype){
			udev_enumerate_add_match_property(enumerate, "DEVTYPE", devtype);
		}
		udev_enumerate_scan_devices(enumerate);
		struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
		struct udev_list_entry *entry;
		udev_list_entry_foreach(entry, devices){
			const char *path = udev_list_entry_get_name(entry);
			struct udev_device *in = udev_device_new_from_syspath(udev, path);
			if(in){
//				std::cout << path << std::endl;
				deviceInfo *info=new deviceInfo(path);
				info->scan(in);
				udev_device_unref(in);
				cat.insert(info);
			}
		}
		udev_enumerate_unref(enumerate);
		return cat;
	}

//	Strings inputTypes={"input"};
	int enumUSB(){
		udev *udev = udev_new();
		usbList=udevCat(udev,"usb","usb_device");
		for(auto in:usbList){
			if (in->values.count("ID_USB_INTERFACES")){
				std::string interfaces=in->values["ID_USB_INTERFACES"];
				std::cout << interfaces << std::endl;
			}
			if (in->values.count("ID_MODEL")){
				std::string model=in->values["ID_MODEL"];
				std::cout << model << std::endl;
			}
/*			
			int fd=open(in->devPath.c_str(),O_RDONLY);
			if(fd<0){
				continue;
			}
			std::cout << "open is " << in->devPath << std::endl;
			close(fd);
*/		
		}
		udev_unref(udev);
		return 0;
	}

//	"ID_INPUT_KEYBOARD", ID_INPUT_KEY, ID_INPUT_MOUSE, ID_INPUT_TOUCHPAD, ID_INPUT_TOUCHSCREEN, ID_INPUT_TABLET, ID_INPUT_JOYSTICK, ID_INPUT_ACCELEROMETER

	int enumInputs(const char *subsystem){
		deviceInfo *kb=NULL;
		deviceInfo *mouse=NULL;
		std::stringstream ss;
		udev *udev = udev_new();
		inputList=udevCat(udev,subsystem);
		for(auto in:inputList){
			int fd=open(in->devPath.c_str(),O_RDONLY);
			if(fd<0){
				continue;
			}
//			std::cout << "open is " << in->devPath << std::endl;
			unsigned char axes=0;
			unsigned char buttons=0;
			int ares=ioctl (fd, JSIOCGAXES, &axes);
			int bres=ioctl (fd, JSIOCGBUTTONS, &buttons);
			if(ares==0 || bres==0){
				if (axes) in->addChannel("axes",(int)axes,0);
				if (buttons) in->addChannel("buttons",(int)buttons,0);
			}
#ifdef KB_STUFF
			input_id gid;
			int gres=ioctl(fd,EVIOCGID,&gid);
			if(gres==0){
				if(in->valueEquals("ID_INPUT","1")){
//					if(in->valueEquals("ID_INPUT_KEY","1")){
					if(in->valueEquals("ID_INPUT_KEYBOARD","1")){
						kb=kb?kb:in;
						std::cout << "KB" << std::endl;
//							in->attributes()
//							ss << "INPUT KB:" << gid.bustype << "," << gid.product << " v" << gid.version << std::endl;
//							in->dump(ss);
					}
					if(in->valueEquals("ID_INPUT_MOUSE","1")){
						mouse=mouse?mouse:in;														
						std::cout << "MOUSE" << std::endl;
					}
					if(in->valueEquals("ID_MODEL","Raspberry_Pi_Internal_Keyboard")){
						continue; //bork bork
					}
					in->dump();					
				}else{
					std::cout << "INPUT CGID:" << gid.bustype << "," << gid.product << " v" << gid.version << std::endl;
				}
			}
#endif
			char buffer[256];
			int res=ioctl(fd,EVIOCGNAME(256),buffer);
			if(res==0){
				in->values["CGNAME"]=buffer;
//				ss << "\tCGNAME=" << buffer << std::endl;
			}

			close(fd);
		}
		udev_unref(udev);

		return 0;
	}
};
