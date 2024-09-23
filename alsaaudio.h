#pragma once

#include "nitro.h"
#include "synth.h"
#include "json.h"

#include <set>

#include <iomanip>
#include <cmath>

#include <sys/ioctl.h>
#include <sys/stat.h>

#ifndef USE_ALSA_AUDIO
#error "USE_ALSA_AUDIO not defined"
#endif

#include <alsa/asoundlib.h>

#define SHOW_CTL_INFO

std::string safeString(const char *s){
	return(s==NULL)?std::string(""):std::string(s);
}

struct PCMCapabilities{
	std::string name;
	uint32_t minOutputs;
	uint32_t maxOutputs;
	uint32_t minSampleRate;
	uint32_t maxSampleRate;
	snd_pcm_uframes_t minPeriod;
	snd_pcm_uframes_t maxPeriod;
	uint32_t outputFormats;
	uint32_t minInputs;
	uint32_t maxInputs;
	uint32_t minRecordRate;
	uint32_t maxRecordRate;
	uint32_t inputFormats;

	PCMCapabilities(std::string name):name(name){
	}

	void reset(){
		minOutputs=0;
		maxOutputs=0;
		minSampleRate=0;
		maxSampleRate=0;
		minPeriod=0;
		maxPeriod=0;
		outputFormats=0;
		minInputs=0;
		maxInputs=0;
		minRecordRate=0;
		maxRecordRate=0;
		inputFormats=0;
	}

	int pcmFormats(snd_pcm_hw_params_t *params){
		snd_pcm_format_mask_t *mask;
		snd_pcm_format_mask_alloca(&mask);
		snd_pcm_hw_params_get_format_mask(params,mask);
		int result=
			snd_pcm_format_mask_test(mask,SND_PCM_FORMAT_S16_LE)?1:0|
			snd_pcm_format_mask_test(mask,SND_PCM_FORMAT_S32_LE)?2:0;
		return result;
	}

	int queryRecord(){
		snd_pcm_t *handle=NULL;
		int pcmError=snd_pcm_open(&handle,name.c_str(),SND_PCM_STREAM_CAPTURE,0);
		if(pcmError){
			if(pcmError!=-2){
				std::cout << "ALSA AudioDevice::queryRecord snd_pcm_open error " << pcmError << ", " << snd_strerror(pcmError) << std::endl;
			}
			return 1;
		}
		snd_pcm_hw_params_t *params=NULL;
		snd_pcm_hw_params_alloca(&params);
		snd_pcm_hw_params_any(handle, params);
		snd_pcm_hw_params_get_channels_min(params,&minInputs);
		snd_pcm_hw_params_get_channels_max(params,&maxInputs);
		int dir;
		snd_pcm_hw_params_get_rate_min(params,&minRecordRate,&dir);
		snd_pcm_hw_params_get_rate_max(params,&maxRecordRate,&dir);
		inputFormats=pcmFormats(params);
		snd_pcm_close(handle);
		return 0;
	}

	int queryPlayback(){
		snd_pcm_t *handle=NULL;
		snd_pcm_hw_params_t *params=NULL;
		int pcmError=snd_pcm_open(&handle,name.c_str(),SND_PCM_STREAM_PLAYBACK,0);
		if(pcmError){
			if(pcmError==-2) return 1;
			if(pcmError==-19) return 2;
			std::cout << "ALSA AudioDevice::queryPlayback snd_pcm_open error " << pcmError << ", " << snd_strerror(pcmError) << std::endl;
			return 3;
		}
		snd_pcm_hw_params_alloca(&params);
		snd_pcm_hw_params_any(handle, params);
		snd_pcm_hw_params_get_channels_min(params,&minOutputs);
		snd_pcm_hw_params_get_channels_max(params,&maxOutputs);
		int dir;
		snd_pcm_hw_params_get_rate_min(params,&minSampleRate,&dir);
		snd_pcm_hw_params_get_rate_max(params,&maxSampleRate,&dir);
		snd_pcm_hw_params_get_period_size_min(params,&minPeriod,&dir);
		snd_pcm_hw_params_get_period_size_max(params,&maxPeriod,&dir);
		outputFormats=pcmFormats(params);
		snd_pcm_close(handle);
		return 0;
	}

	std::string toJSON(){
		std::stringstream ss;
		ss<<"{\"name\":"<<quoted(name);
//		ss<<",\"description\":"<<quoted(description);
		ss<<",\"minOutputs\":"<<minOutputs,
		ss<<",\"maxOutputs\":"<<maxOutputs,
		ss<<",\"minSampleRate\":"<<minSampleRate;
		ss<<",\"maxSampleRate\":"<<maxSampleRate;
		ss<<",\"minPeriod\":"<<minPeriod;
		ss<<",\"maxPeriod\":"<<maxPeriod;
		ss<<",\"inputFormats\":"<<inputFormats;
		ss<<",\"minInputs\":"<<minInputs,
		ss<<",\"maxInputs\":"<<maxInputs,
		ss<<",\"minRecordRate\":"<<minRecordRate;
		ss<<",\"maxRecordRate\":"<<maxRecordRate;
		ss<<",\"outputFormats\":"<<outputFormats;
		ss<<"}";
		return ss.str();
	}
};

struct ALSADevice{
	std::string name;
	std::string title;
	std::string info;
	int flags; //1 playback 2 record 
	PCMCapabilities *pcmCaps;

	ALSADevice(std::string name,std::string title,std::string info,int flags):name(name),title(title),info(info),flags(flags){
	}

	int query(){
		int activeFlags=0;
		pcmCaps=new PCMCapabilities(name);
		if(flags&1){
			int playback=pcmCaps->queryPlayback();
			if(playback==0){
				activeFlags|=1;
			}
		}
		if(flags&2){
			int record=pcmCaps->queryRecord();
			if(record==0){
				activeFlags|=2;
			}
		}
		return activeFlags;
	}

	std::string toJSON(){
		std::stringstream ss;
		ss<<"{\"name\":"<<quoted(name);
		ss<<",\"title\":"<<quoted(title);
		ss<<",\"info\":"<<quoted(info);
		ss<<",\"flags\":"<<flags;
		ss<<"}";
		return ss.str();
	}

};


struct ALSAAudio{
	std::vector<ALSADevice> alsaDevices;

	ALSAAudio(){
	}

	void about(){
		std::cout << "ALSA library version:" << SND_LIB_VERSION_STR << std::endl;
	}

	int enumCards(){
		char cardName[32];
		snd_ctl_card_info_t *cardInfo;
		snd_ctl_card_info_alloca(&cardInfo);

		snd_pcm_info_t *pcmInfo;
		snd_pcm_info_alloca(&pcmInfo);

		snd_rawmidi_info_t *midiInfo;
		snd_rawmidi_info_alloca(&midiInfo);

		snd_pcm_hw_params_t *params=NULL;
		snd_pcm_hw_params_alloca(&params);

		int cardIndex=-1;
		while(true){
			snd_card_next(&cardIndex);
			if(cardIndex<0) break;
			snprintf(cardName,sizeof(cardName),"hw:%d",cardIndex);
			snd_ctl_t *handle;
			int ctl=snd_ctl_open(&handle,cardName,0);
			if(ctl){
				std::cout << "snd_ctl_open fail " <<ctl<<std::endl;
				continue;
			}
			int info=snd_ctl_card_info(handle,cardInfo);
			if(info){
				std::cout << "snd_ctl_card_info fail " <<info<<std::endl;
				continue;
			}
			const char *name= snd_ctl_card_info_get_name(cardInfo);
			const char *longname= snd_ctl_card_info_get_longname(cardInfo);

#ifdef SHOW_POWER
			unsigned int powerState;
			int power=snd_ctl_get_power_state(handle, &powerState);
			if(power==0){
				std::cout << (powerState?"On":"Off") << std::endl;
			}
#endif

#ifdef SHOW_CTL_INFO
			std::cout << snd_ctl_card_info_get_mixername(cardInfo) << std::endl;
			std::cout << snd_ctl_card_info_get_components(cardInfo) << std::endl;
			std::cout << snd_ctl_card_info_get_driver(cardInfo) << std::endl;
			std::cout << snd_ctl_card_info_get_id(cardInfo) << std::endl;
#endif
			int flags=0;

			// query pcm support
			int pcmdev=-1;
			while(true){
				int getNext=snd_ctl_pcm_next_device(handle,&pcmdev);
				if(getNext || pcmdev<0){
					break;
				}

				snd_pcm_info_set_device(pcmInfo, pcmdev);
				snd_pcm_info_set_subdevice(pcmInfo, 0);

				snd_pcm_info_set_stream(pcmInfo, SND_PCM_STREAM_PLAYBACK);
				int pcmPlay=snd_ctl_pcm_info(handle, pcmInfo);
				if(pcmPlay){
					std::cout << "snd_ctl_pcm_info playback fail " << pcmPlay << std::endl;
				}else{
					int n=snd_pcm_info_get_subdevices_count(pcmInfo);
					std::cout << "PCM IN #" << n << " dev:"<< pcmdev << std::endl;
					flags|=1;
				}

				snd_pcm_info_set_stream(pcmInfo, SND_PCM_STREAM_CAPTURE);
				int pcmRecord=snd_ctl_pcm_info(handle, pcmInfo);
				if(pcmRecord){
//					std::cout << "snd_ctl_pcm_info record fail "<<pcmRecord<<std::endl;
				}else{
					int n=snd_pcm_info_get_subdevices_count(pcmInfo);
//					std::cout << "PCM OUT #"<<n<<" dev:"<<dev<<std::endl;
					flags|=2;
				}
			}

			// query midi support
			int mididev=-1;
			while(true){
				int getNext=snd_ctl_rawmidi_next_device(handle,&mididev);
				if(getNext || mididev<0){
					break;
				}
				snd_rawmidi_info_set_device(midiInfo, mididev);
				snd_rawmidi_info_set_subdevice(midiInfo, 0);

				snd_rawmidi_info_set_stream(midiInfo, SND_RAWMIDI_STREAM_OUTPUT);
				int midiOut=snd_ctl_rawmidi_info(handle, midiInfo);
				if(midiOut){
					std::cout << "snd_ctl_rawmidi_info midi output fail "<<midiOut<<std::endl;					
				}else{
					flags|=4;
				}

				snd_rawmidi_info_set_stream(midiInfo, SND_RAWMIDI_STREAM_INPUT);
				int midiIn=snd_ctl_rawmidi_info(handle, midiInfo);
				if(midiIn){
					std::cout << "snd_ctl_rawmidi_info midi input fail "<<midiIn<<std::endl;					
				}else{
					flags|=8;
				}

			}

			std::cout << cardName << " " << flags  << " " << name << " - " << longname << std::endl;
			alsaDevices.emplace_back(cardName,name,longname,flags);
//		snd_pcm_hw_params_any(handle, params);
			snd_ctl_close(handle);
		}
		return 0;
	}

	int recordPCM(std::string name,std::string &result){
		snd_pcm_t *handle;
		snd_pcm_hw_params_t *params;
		int pcmError=snd_pcm_open(&handle,name.c_str(),SND_PCM_STREAM_CAPTURE,0);
		if(pcmError){
			std::cout << "snd_pcm_open error " << pcmError << ", " << snd_strerror(pcmError) << std::endl;
			return 1;
		}
		snd_pcm_hw_params_alloca(&params);
		int error=0;
		error=snd_pcm_hw_params_any(handle, params);
		if(error) std::cout << "snd_pcm_hw_params_any failure : " << error << std::endl;
		error=snd_pcm_hw_params_set_access(handle,params,SND_PCM_ACCESS_RW_INTERLEAVED);
		if(error) std::cout << "snd_pcm_hw_params_set_access failure : " << error << std::endl;
		error=snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_S16_BE);	//LE
		if(error) std::cout << "snd_pcm_hw_params_set_format failure : " << error << std::endl;
		error=snd_pcm_hw_params_set_channels(handle,params,2);
		if(error) std::cout << "snd_pcm_hw_params_set_channels failure : " << error << std::endl;
		error=snd_pcm_hw_params_set_rate(handle,params,44100,SND_PCM_STREAM_CAPTURE);
		if(error) std::cout << "snd_pcm_hw_params_set_rate failure : " << error << std::endl;

		int hwParams=snd_pcm_hw_params(handle,params);
		if(hwParams){
			std::cout << "snd_pcm_hw_params failure : " << hwParams << std::endl;
		}
		snd_pcm_uframes_t frames;
		snd_pcm_hw_params_get_period_size(params,&frames,NULL);
		std::cout << "testPCMIn " << name << " frame:" << frames << std::endl;
		AudioBuffer buffer(frames,2);
		for (int i = 0; i < 1000; i++) {
			int readCount=snd_pcm_readi(handle,buffer.pcmBuffer,frames);
			double power=buffer.area();
			if(readCount<0){
				std::cout << "!@!" << std::endl;
			}
//			std::cout << readCount << ":" << power << std::endl;
		}
		std::cout << "recordPCM complete" << std::endl; 
		int pcmClose=snd_pcm_close(handle);
		if(pcmClose){
			std::cout << "snd_pcm_close returned " << pcmClose << std::endl;
		}
		return 0;
	}

	int testPCMIn(std::string name,std::string &result){
		snd_pcm_t *handle;
		snd_pcm_hw_params_t *params;
		int pcmError=snd_pcm_open(&handle,name.c_str(),SND_PCM_STREAM_CAPTURE,0);
		if(pcmError){
			std::cout << "snd_pcm_open error " << pcmError << ", " << snd_strerror(pcmError) << std::endl;
			return 1;
		}
		snd_pcm_hw_params_alloca(&params);
		snd_pcm_hw_params_any(handle, params);
		snd_pcm_hw_params_set_access(handle,params,SND_PCM_ACCESS_RW_INTERLEAVED);
		snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_S16_LE);
		snd_pcm_hw_params_set_channels(handle,params,2);
		snd_pcm_hw_params_set_rate(handle,params,48000,SND_PCM_STREAM_CAPTURE);
		int hwParams=snd_pcm_hw_params(handle,params);
		if(hwParams){
			std::cout << "snd_pcm_hw_params failure : " << hwParams << std::endl;
		}
		snd_pcm_uframes_t frames;
		snd_pcm_hw_params_get_period_size(params,&frames,NULL);
		std::cout << "testPCMIn " << name << " frame:" << frames << std::endl;
		AudioBuffer buffer(frames,2);
		for (int i = 0; i < 1000; i++) {
			int readCount=snd_pcm_readi(handle,buffer.pcmBuffer,frames);
			double power=buffer.area();
			std::cout << readCount << ":" << power << std::endl;
		}
		int pcmClose=snd_pcm_close(handle);
		if(pcmClose){
			std::cout << "snd_pcm_close returned " << pcmClose << std::endl;
		}
		return 0;
	}

	int testPCMOut(std::string name,std::string &result){
		std::cout << "testAudioOut snd_pcm_open " << name << std::endl;

		snd_pcm_t *handle;
		snd_pcm_hw_params_t *params;

		int pcmError=snd_pcm_open(&handle,name.c_str(),SND_PCM_STREAM_PLAYBACK,0);

		if(pcmError){
			std::cout << "snd_pcm_open error " << pcmError << ", " << snd_strerror(pcmError) << std::endl;
			return 1;
		}

	//	const char *name=snd_pcm_name(handle);

		snd_pcm_hw_params_alloca(&params);
		snd_pcm_hw_params_any(handle, params);

		snd_pcm_hw_params_set_access(handle,params,SND_PCM_ACCESS_RW_INTERLEAVED);
		snd_pcm_hw_params_set_format(handle,params,SND_PCM_FORMAT_S16_LE);
		snd_pcm_hw_params_set_channels(handle,params,2);
		snd_pcm_hw_params_set_rate(handle,params,48000,SND_PCM_STREAM_PLAYBACK);
		int hwParams=snd_pcm_hw_params(handle,params);
		if(hwParams){
			std::cout << "snd_pcm_hw_params failure : "<<hwParams<<std::endl;
			return 2;
		}

		snd_pcm_uframes_t frames;
		snd_pcm_hw_params_get_period_size(params,&frames,NULL);
		std::cout << "period size:"<<frames<<std::endl;
#ifdef HAS_ALSA_AUDIO
		SynthStream source(frames,2);
		for(int i=0;i<350;i++){
			source.mixDown(handle);
		}
		snd_pcm_drain(handle);
#endif
		int pcmClose=snd_pcm_close(handle);
		if(pcmClose){
			std::cout << "snd_pcm_close returned " << pcmClose << std::endl;
		}
		
		return 0;
	}


// https://chromium.googlesource.com/chromium/src/+/32352ad08ee673a4d43e8593ce988b224f6482d3/media/midi/midi_manager_alsa.cc

	int captureMidiIn(snd_seq_t *seq){
		snd_seq_addr_t sender={64,0};
		snd_seq_addr_t dest={128,0};
		snd_seq_port_subscribe_t *subs;
		
//		snd_seq_port_subscribe_malloc(&subs);
		snd_seq_port_subscribe_alloca(&subs);
		snd_seq_port_subscribe_set_sender(subs, &sender);
		snd_seq_port_subscribe_set_dest(subs, &dest);
		snd_seq_port_subscribe_set_queue(subs, 1);
		snd_seq_port_subscribe_set_time_update(subs, 1);
		snd_seq_port_subscribe_set_time_real(subs, 1);
		int result=snd_seq_subscribe_port(seq, subs);

		if(result<0){
			std::cout << "captureMidiIn snd_seq_subscribe_port error " << result << " " << snd_strerror(result) << std::endl;
		}
		return result;
	}

	int testMidiIn(std::string name,std::string &result){
		std::cout << "testMidiIn snd_seq_open " << name << std::endl;
		snd_seq_t *handle;
		int seqError=snd_seq_open(&handle,name.c_str(),SND_SEQ_OPEN_INPUT,0);
		if(seqError){
			std::cout << "snd_seq_open error " << seqError << ", " << snd_strerror(seqError) << std::endl;
			return 1;
		}
		snd_seq_set_client_name(handle, "ALSA Sequencer Test");
		int inCaps=SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_NO_EXPORT;
		int inType=SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION;
		int inPort=snd_seq_create_simple_port(handle,NULL,inCaps,inType);
//		int in=snd_seq_create_simple_port(handle,"listen:in",SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,SND_SEQ_PORT_TYPE_APPLICATION);
//		int in=snd_seq_create_simple_port(handle,"listen:in",SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,SND_SEQ_PORT_TYPE_MIDI_GENERIC);
		captureMidiIn(handle);
		while(1){
		    snd_seq_event_t *ev = NULL;
   		 	snd_seq_event_input(handle, &ev);
		}
		snd_seq_close(handle);
		return 0;
	}
};
