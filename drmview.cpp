// drmview.cpp

#include "drmdisplay.h"
#include "eglgraphics.h"
#include "alsaaudio.h"
#include "udev.h"
//#include "host.h"
#include "sysinfo.h"
#include "cec.h"
//#include "video.h"

std::string ShadersPath="../shaders";

int flipTest(DRMDisplay &display);
int displayTest();

enum TestDisplays{
	TestDisplaysSetMode=100,
	TestDisplaysBegin=200,
	TestDisplaysConfigure=300,
	TestDisplaysInit=400,
	TestDisplaysEnd=500
};

int enumAudio(){
	ALSAAudio audio;
	ALSADevice *pcmOut=0;//&audio.alsaDevices[2];
	ALSADevice *midiIn=0;//&audio.alsaDevices[1];

	for(ALSADevice &dev:audio.alsaDevices){
		int flags=dev.query();
		std::cout << dev.flags << "," << flags << std::endl;
		if(!pcmOut && flags&1){
			pcmOut=&dev;
		}
		if(!midiIn && flags&8){
			midiIn=&dev;
		}
	}
	std::string result;
	return 0;
}


int main(){
//	return test3D();
//	return testCEC();
//	return testVideo();

	SysInfo sysInfo;

	Attributes registry={
		{"hostname",sysInfo.hostname},
		{"version","0.0.6"},
		{"machine",sysInfo.machine()},
		{"domain",sysInfo.domain()},
		{"user",getenv("USER")},
		{"serialnumber",sysInfo.serialnumber}
	};

//	int hosting=hostIPv6(Broadcom,registry);

	Devices devices;

//	devices.enumInputs("input");
//	devices.enumUSB();

	ALSAAudio audio;
	audio.about();
	int err=audio.enumCards();

	std::string result;
//	audio.testPCMIn("hw:3",result);
//	audio.testPCMOut("hw:CARD=vc4hdmi0,DEV=0",result);
	audio.testPCMOut("default",result);
//	audio.testPCMOut("hw:0,3",result);
//	audio.testMidiIn("default",result);

	return displayTest();
}

int displayTest(){
	DRMDisplay display;

	int notok=0;	
	notok=display.openCard("/dev/dri/card1");
	if(notok){
		notok=display.openCard("/dev/dri/card0");
	}
	if(notok){
		return notok;
	}

	std::string result="{}";
	int infoerror=display.deviceInfo(result);
	if(infoerror){
		return infoerror;
	}
//	std::cout << result << std::endl;

	int mode=0;//display.countModes()-1;
	int setmode=display.setMode(mode);
	if(setmode){
		return TestDisplaysConfigure+setmode;
	}
	std::cout << "mode set" << std::endl;

	int begin=display.beginGraphics();
	if(begin){
		return TestDisplaysBegin+begin;
	}
	std::cout << "Display graphics begun" << std::endl;

	int configure=display.configure();
	if(configure){
		return TestDisplaysConfigure+configure;
	}
	std::cout << "display configured" << std::endl;

	flipTest(display);

	int endGraphics=display.endGraphics();
	if(endGraphics){
		return TestDisplaysConfigure+configure;
	}
	std::cout << "display graphics ended" << std::endl;

	int destroy=display.destroy();
	if(destroy){
		return destroy;
	}
	std::cout << "display destroyed" << std::endl;

	return 0;
}

int flipTest(DRMDisplay &display){
	EGLGraphics graphics;

	int testCount=120;
	if(testCount){

		int init=graphics.initEGL(ShadersPath);
		if(init){
			return TestDisplaysInit+init;
		}

		std::cout << "Graphics Initialised:"<< display.width << "x" <<display.height << std::endl;
		for(int i=0;i<testCount;i++){
			graphics.clear(0.15f, 0.15f, (float)i/testCount, 1.0f);
			int flip=display.flip();
			if(flip){
				std::cout << "flip error:" << flip << std::endl;
			}
			std::cout << ".";
//			sleep(1);
		}

		display.flop();
	}
	return 0;
}
