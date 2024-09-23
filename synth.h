#pragma once

#include "nitro.h"
#include <set>
#include <cmath>

#define AudioFrequency 48000

#ifdef USE_ALSA_AUDIO
#include <alsa/asoundlib.h>
#endif

typedef float V; //voltage
typedef float F; //frequency
typedef float T; //time

struct Oscillator{
	T delta=0.0;
	virtual V sample(F hz){
		return 0.0;
	}
};

struct Square:Oscillator{
	V sample(F hz){
		delta+=hz/AudioFrequency;
		return -1+2*(int(delta)&1);
	}
};

struct Sine:Oscillator{
	V sample(F hz){
		delta+=hz/AudioFrequency;
		return sinf(2*M_PI*delta);
	}
};

struct Saw:Oscillator{
	V sample(F hz){
		delta+=hz/AudioFrequency;
		return fmod(delta+1,2.0)-1;
	}
};

struct Triangle:Oscillator{
	V sample(F hz){
		delta+=hz/AudioFrequency;
		return fabs(fmod(delta,2.0))-1;
	}
};

struct Noise:Oscillator{
	V a=0.5;
	V sample(F hz){
		T d=delta;
		delta+=hz/AudioFrequency;
		if(int(d)!=int(delta)){
			a=float(rand())/RAND_MAX;
		}
		return 1-2*a;
	}
};

struct Envelope{
	virtual V On(){return 1.0;}
	virtual V Off(){return 0.0;}
};

struct ADSR:Envelope{
	T attack;
	T decay;
	V sustain;
	T release;

	bool noteOn=false;
	T t=0.0;
	V value=0.0;

	ADSR(T attack,T decay,V sustain,T release):attack(attack),decay(decay),sustain(sustain),release(release){		
	}

	V On(){
		if(!noteOn){
			t=0.0;
			noteOn=true;
		}
		t+=1.0/AudioFrequency;
		V v=sustain;
		if (t<attack){
			v=t/attack;
		}else if(t-attack<decay){
			v=1.0-(1-sustain)*((t-attack)/decay);
		}
		value=v;
		return v;
	}

	V Off(){
		if(noteOn){
			t=0.0;
			noteOn=false;
		}
		t+=1.0/AudioFrequency;
		return (t<release)?value*(1.0-t/release):0.0;
	}
};

struct Voice{
	Oscillator *osc;
	Envelope *env;
	F hz=0.0;
	V pan=0.0;
	V amp=0.4;
	bool on=false;

	Voice(Oscillator *osc,Envelope *env):osc(osc),env(env){		
	}

	void playNote(int note){
		hz=440.0*powf(2.0,(note-67.0)/12);
		on=true;
	}

	void stopNode(){
		on=false;
	}

	void stereoMix(float *buffer,int samples){
		V left=1.0;
		V right=1.0;
		if(pan<0) right+=pan;
		if(pan>0) left-=pan;
		for(int i=0;i<samples;i++){
			V v=osc->sample(hz)*amp;
			V e=on?env->On():env->Off();
			if(e==0.0) {
				osc->delta=0.0;
			}else{
				buffer[i*2+0]+=e*left*v;
				buffer[i*2+1]+=e*right*v;
			}
		}
	}

};

struct AudioBuffer{
	int frames;
	int channels;
	int16_t *pcmBuffer;

	AudioBuffer(int frames,int channels):frames(frames),channels(channels){
		int count=frames*channels*2;
		pcmBuffer=new int16_t[count];
	}

	~AudioBuffer(){
		delete[] pcmBuffer;
	}

	double area(){
		double total;
		for(int i=1;i<frames;i++){
			int ldiff=(int)pcmBuffer[i*2+0]-pcmBuffer[(i-1)*2+0];
			int rdiff=(int)pcmBuffer[i*2+1]-pcmBuffer[(i-1)*2+1];
			total+=ldiff*ldiff+rdiff*rdiff;
		}
		return total;
	}
};

struct SynthStream:AudioBuffer{
	V *mixBuffer;
	std::set<Voice*> voices;

	SynthStream(int frames,int channels):AudioBuffer(frames,channels){
		int count=frames*channels;
		mixBuffer=new float[count];
		playNote(60);
		playNote(64);
		playNote(67);
	}

	~SynthStream(){
		delete []mixBuffer;
	}

	void playNote(int note){
		Voice *voice=new Voice(new Square(),new ADSR(0.06,0.01,0.92,0.2));//0.001,1.5,0.8,0.5));
		voice->playNote(note);
		voices.insert(voice);
	}
#ifdef USE_ALSA_AUDIO
	int mixDown(snd_pcm_t *pcm){
		int count=frames*channels;
		memset(mixBuffer,0,count*4);
		for(Voice*voice:voices){
			voice->stereoMix(mixBuffer,frames);
		}
		for(int i=0;i<count;i++){
			// todo: limit float before conversion
			float f=mixBuffer[i];
			f=f<-1.0?-1.0:f>1.0?1.0:f;
			pcmBuffer[i]=int16_t(f*32767);
		}
		int wrote=snd_pcm_writei(pcm,pcmBuffer,frames);
//		std::cout << "snd_pcm_writei" << wrote << std::endl;
// todo: write remainder on next call
		return wrote;
	}
#endif	
};
