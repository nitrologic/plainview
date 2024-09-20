#pragma once

#include <libcec/cec.h>

static CEC::libcec_configuration client_cec_config;
static CEC::libcec_configuration adapter_cec_config;
static CEC::ICECCallbacks cec_callbacks;

static void cb_cec_keypress(void* lib, const CEC::cec_keypress* key){
	printf("cec keypress\n");
}

static void cb_cec_log_message(void* lib, const CEC::cec_log_message* message){
	const char* strLevel="";
	switch (message->level)
	{
		case CEC::CEC_LOG_ERROR:
			strLevel = "ERROR:   ";
			break;
		case CEC::CEC_LOG_WARNING:
			strLevel = "WARNING: ";
			break;
		case CEC::CEC_LOG_NOTICE:
			strLevel = "NOTICE:  ";
			break;
		case CEC::CEC_LOG_TRAFFIC:
			strLevel = "TRAFFIC: ";
			break;
		case CEC::CEC_LOG_DEBUG:
			strLevel = "DEBUG:   ";
			break;
		default:
			break;
	}

//	std::cout << strLevel << std::endl;
//, message->time, 
	std::cout << "cec "<<strLevel<<std::endl;
	std::cout << message->message << std::endl;
}

int testCEC(){

	client_cec_config.Clear();
	client_cec_config.clientVersion = _LIBCEC_VERSION_CURRENT;
	client_cec_config.bActivateSource = 0;
	snprintf(client_cec_config.strDeviceName, sizeof(client_cec_config.strDeviceName), "StereoBASIC");	

	cec_callbacks.Clear();
	cec_callbacks.logMessage=cb_cec_log_message;
	cec_callbacks.keyPress=cb_cec_keypress;
	client_cec_config.callbacks = &cec_callbacks;

	client_cec_config.deviceTypes.Add(CEC::CEC_DEVICE_TYPE_RECORDING_DEVICE);

	CEC::ICECAdapter*cec_adapter=CECInitialise(&client_cec_config);
//	printf("adapter ComName %s\n",adapter->strComName);

	bool updated=cec_adapter->GetCurrentConfiguration(&adapter_cec_config);
	printf("adapter_cec_config name %s\n",adapter_cec_config.strDeviceName);

// Get a cec adapter by initialising the cec library

    CEC::cec_adapter_descriptor devices[10];
    int8_t devices_found = cec_adapter->DetectAdapters(devices, 10, nullptr, true /*quickscan*/);
    if( devices_found <= 0)
    {
        std::cerr << "Could not automatically determine the cec adapter devices\n";
        return 1;
    }
	printf("found %d devices\n",devices_found);
	printf("device 0 com is %s\n",devices[0].strComName);

    // Open a connection to the zeroth CEC device
    if( !cec_adapter->Open(devices[0].strComName) )
    {        
        std::cerr << "Failed to open the CEC device on port " << devices[0].strComName << std::endl;
        return 1;
    }

	uint8_t state=cec_adapter->AudioToggleMute();
	printf("audio status:%u\n",state);

	return 0;
//	bool open=adapter->Open("TV");
}
