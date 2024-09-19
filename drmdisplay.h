// drmdisplay.h

#pragma once

#include "nitro.h"

#include <vector>

#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

static std::string FourCC(int id){
	uint8_t bytes[4];
	*(int*)bytes=id;
	std::stringstream ss;
	if(bytes[0]>31&&bytes[1]>31&&bytes[2]>31&&bytes[3]>31){
		ss<<bytes[0]<<" "<<bytes[1]<<" "<<bytes[2]<<" "<<bytes[3];
	}else{
		ss<<std::hex<<(int)bytes[0]<<" "<<(int)bytes[1]<<" "<<(int)bytes[2]<<" "<<(int)bytes[3];
	}
	return ss.str();
}

const char *glString(GLenum name){
	const GLubyte *bytes=glGetString(name);
	return bytes?(const char*)bytes:"";
}

const EGLint egl_framebuffer_attribs[]={
	EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
	EGL_CONFORMANT, EGL_OPENGL_ES3_BIT,
	EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	EGL_RED_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_BLUE_SIZE, 8,
	EGL_ALPHA_SIZE, 8,
	EGL_TRANSPARENT_TYPE, EGL_NONE,
	EGL_DEPTH_SIZE, 16,
	EGL_STENCIL_SIZE, 8,
	EGL_LEVEL,0,
	//EGL_SAMPLE_BUFFERS, sampleBuffer,
	//EGL_SAMPLES, samples,
	EGL_NONE
};

static const EGLint egl_context_attributes[]={
	EGL_CONTEXT_CLIENT_VERSION, 3,
	EGL_NONE
};


struct DRMDisplay{
	int fd;
	drmModeRes *resources;
	std::vector<drmModeConnectorPtr> connectors;
	std::vector<uint32_t> connectorIds;

	int width;
	int height;

	drmModeModeInfo modeInfo;
//	drmModeEncoderPtr encoder
	gbm_surface *surface;

	gbm_bo *prevBO=NULL;
	uint32_t prevFB=0;
	int crtcId;

	gbm_device *gbmDevice;
	gbm_surface *gbmSurface;

	EGLDisplay eglDisplay;
	EGLint major,minor;
	EGLConfig eglConfig;
	EGLContext eglContext;
	EGLSurface eglSurface;

	DRMDisplay():fd(-1){
	}

	int openCard(const char *driPath){
		int ok=drmAvailable();
		if(!ok){
			std::cout << "drmAvailable false" << std::endl;
			return 1;
		}

		fd=::open(driPath, O_RDWR);
		FDWarnCheck(fd,2,"open dri renderer failure");

		resources = drmModeGetResources(fd);
		if (!resources){
			std::cout << "drmModeGetResources failure" << std::endl;
			::close(fd);
			return 3;
		}

		int n=resources->count_connectors;
		std::cout << "DRMDisplay connectors = " << n << std::endl;

		for(int i=0;i<n;i++){
			drmModeConnector *connector = drmModeGetConnector(fd, resources->connectors[i]);
			if ((connector->connection == DRM_MODE_CONNECTED) && (connector->encoder_id)){
				connectors.push_back(connector);
				connectorIds.push_back(connector->connector_id);
			}
		}

		if(connectors.empty()){
			std::cout << "drmModeGetConnector found no connected connectors" << std::endl;
			closeCard();
			return 4;
		}

//#define CALL_DRM_MASTER 		
#ifdef CALL_DRM_MASTER
		int master=drmSetMaster(fd);
		if(master<0){
			std::cout << "drmSetMaster returned "<<master<<std::endl;
			drmModeFreeResources(resources);
			resources=NULL;
			closeCard();
			return 5;
		}
		std::cout << "drmSetMaster success" << std::endl;
#endif

		gbmDevice=gbm_create_device(fd);
		if(gbmDevice==NULL){
			std::cout << "gbm_create_device failure" << std::endl;
			return 6;
		}

#ifdef EGL_PLATFORM_GBM_KHR
		eglDisplay=eglGetPlatformDisplay(EGL_PLATFORM_GBM_KHR,gbmDevice,NULL);
#else
		eglDisplay=eglGetDisplay((EGLNativeDisplayType)gbmDevice);
#endif
//		eglDisplay=eglGetPlatformDisplay(EGL_PLATFORM_GBM_KHR,gbmDevice,NULL);
//		eglDisplay=eglGetDisplay((EGLNativeDisplayType)gbmDevice);
		if(eglDisplay==NULL){
			std::cout << "eglGetDisplay failure" << std::endl;
			return 7;
		}

		std::cout << "initializing egl" << std::endl;
		if (!eglInitialize(eglDisplay, &major, &minor)) {
			std::cout << "eglInitialize failure" << std::endl;
			return 8;
		}

		std::cout << "egl version " << major << "." << minor << std::endl;

		return 0;
	}

	int closeCard(){
		if(resources){
			drmModeFreeResources(resources);
			resources=NULL;
		}
		if(fd<0){
			std::cout << "DRMDisplay closeCard - device not open error"<<std::endl;
			return 1;
		}
		::close(fd);
		fd=-1;
		return 0;
	}

	int deviceInfo(std::string &result){
		FDWarnCheck(fd,1,"displayInfo failed to open ");
		if(connectors.size()<1){
			std::cout << "displayInfo no connectors error" << std::endl;
			return 2;
		}
		drmModeConnector *connector=connectors[0];		
		bool connected=connector->connection == DRM_MODE_CONNECTED;
		int modes = connector->count_modes;
		if(modes<=0){
			std::cout << "displayInfo no modes on connector 0 found" << std::endl;
			return 3;
		}
		std::stringstream ss;
		ss<<"{\"id\":"<<connector->connector_id<<",\"connected\":"<<(connected?"true":"false") << ",";
	//	std::cout << "connected:" << (connected?"TRUE":"FALSE") << std::endl;
	//	std::cout << "modes:" << modes << std::endl;
		ss<<"\"modes\":[ ";
		for(int i=0;i<modes;i++){
			const drmModeModeInfo *const mode = connector->modes+i;
			ss<<"{\"name\":\""<<mode->name<<"\",";
			ss<<"\"width\":"<<mode->hdisplay<<",";
			ss<<"\"height\":"<<mode->vdisplay<<",";
			ss<<"\"refresh\":"<<mode->vrefresh<<",";
			ss<<"\"flags\":"<<mode->flags<<",";
			ss<<"\"type\":"<<mode->type<<"},";
		}
		ss.seekp(-1,std::ios_base::end);
		ss<<"],";
		drmModeEncoder *encoder = drmModeGetEncoder(fd, connector->encoder_id);
		int encoderType=encoder->encoder_type;
		drmModeCrtc *crtc=drmModeGetCrtc(fd, encoder->crtc_id);
		drmModeFB *fb=drmModeGetFB(fd,crtc->buffer_id);
		ss<<"\"encoder\":{\"type\":"<<encoderType<<",\"fb\":{";
		if(fb){
			ss<<"\"id\":"<<fb->fb_id<<",";
			ss<<"\"width\":"<<fb->width<<",";
			ss<<"\"height\":"<<fb->height<<",";
			ss<<"\"bpp\":"<<fb->bpp;
		}
		ss<<"}}}";
		result=ss.str();
		return 0;
	}

	int countModes(){
		drmModeConnector *connector=connectors[0];		
		int modes = connector->count_modes;
		return modes;	
	}

	int setMode(int modeIndex){
//		std::cout << "drmModeGetConnector found " << connectors.size() << " connected connectors" << std::endl;
		for(drmModeConnectorPtr modeConnector:connectors){
			int modes=modeConnector->count_modes;
			if (modeIndex>=modes){
				continue;
			}
			modeInfo = modeConnector->modes[modeIndex];
			width=modeInfo.hdisplay;
			height=modeInfo.vdisplay;
// encoder
			drmModeEncoderPtr encoder = drmModeGetEncoder(fd, modeConnector->encoder_id);
			if(!encoder){
				std::cout << "drmModeGetEncoder failure" << std::endl;
				closeCard();
				return 1;
			}
			crtcId=encoder->crtc_id;
			drm_mode_crtc crtc={0};
			crtc.crtc_id = encoder->crtc_id;
			int getCTRCError=drmIoctl(fd, DRM_IOCTL_MODE_GETCRTC, &crtc);
			if(getCTRCError){
				drmModeFreeEncoder(encoder);
				std::cout << "DRM_IOCTL_MODE_GETCRTC failure:" << getCTRCError << std::endl;
				closeCard();
				return 2;
			}
			std::cout << "crtcId = " << FourCC(crtcId) << "(" << crtcId << ")" << std::endl;
			drmModeFreeEncoder(encoder);			
		}
		std::cout << "DRMDisplay setMode complete" << std::endl;
		return 0;
	}

	int beginGraphics(){
		std::cout << "DRMDisplay init " << width << "x" << height << std::endl;

/* moved to init

		std::cout << "creating gbm device" << std::endl;
		gbmDevice=gbm_create_device(fd);
		if(gbmDevice==NULL){
			std::cout << "gbm_create_device failure" << std::endl;
			return 1;
		}
*/

// simon come here - kapiti crash crash
		std::cout << "creating gbm surface" << std::endl;
		gbmSurface=gbm_surface_create(gbmDevice,width,height,GBM_FORMAT_ARGB8888,GBM_BO_USE_SCANOUT|GBM_BO_USE_RENDERING);
//		gbmSurface=gbm_surface_create(gbmDevice,width,height,GBM_FORMAT_XRGB8888,GBM_BO_USE_SCANOUT|GBM_BO_USE_RENDERING);
		if(gbmSurface==NULL){
			std::cout << "gbm_surface_create failure" << std::endl;
			return 2;
		}

		std::cout << "eglDisplay " << major << "." << minor << std::endl;

		return 0;
	}

	int endGraphics(){

		EGLBoolean terminated = eglTerminate(eglDisplay);
		if(!terminated) return 1;

		gbm_surface_destroy(gbmSurface);
		gbmSurface=NULL;

//		drmModeFreeCrtc(crtcId);
//		gbm_device_destroy(gbmDevice);
//		gbmDevice=NULL;
//		close();

		int res=closeCard();
		std::cout << "endGraphics closeCard->" << res << std::endl;
		return 0;
	}

	int destroy(){
		EGLBoolean current=eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if(!current){
			std::cout << "eglMakeCurrent failure" << std::endl;
			return 1;
		}
		EGLBoolean destroySurface=eglDestroySurface(eglDisplay,eglSurface);
		if(!destroySurface){
			std::cout << "eglDestroySurface failure" << std::endl;
			return 2;
		}
		eglSurface=NULL;
		EGLBoolean destroyContext=eglDestroyContext(eglDisplay,eglContext);	
		if(!destroyContext){
			std::cout << "eglDestroyContext failure" << std::endl;
			return 3;
		}
		eglContext=NULL;
		return 0;
	}

	int configure(){
		EGLint configCount = 0;
		EGLint matchingCount=0;
		EGLConfig *configs = NULL;

		for(int i=0;i<2;i++){
			EGLBoolean ok = eglGetConfigs(eglDisplay, configs, configCount, &configCount);
			if(!ok){
				std::cout << "eglGetConfigs failure" << std::endl;
				return 1;
			}
			if(!configs) {
				std::cout << "allocated " << configCount << " configs" << std::endl;
				configs=new EGLConfig[configCount];
			}
		}

		EGLBoolean chooseConfig=eglChooseConfig(eglDisplay,egl_framebuffer_attribs,configs, configCount, &matchingCount);
		if(!chooseConfig){
			std::cout << "eglChooseConfig failure" << std::endl;
			return 2;
		}

		EGLint fbConfig=0;
		eglConfig=configs[fbConfig];

		eglBindAPI(EGL_OPENGL_ES_API);

		eglContext=eglCreateContext(eglDisplay,eglConfig,EGL_NO_CONTEXT,egl_context_attributes);
		if(eglContext==EGL_NO_CONTEXT)
		{
			std::cout << "eglCreateContext failure" << std::endl;
			return 6;
		}

		eglSurface=eglCreateWindowSurface(eglDisplay,eglConfig,(EGLNativeWindowType)gbmSurface,NULL);
		if(eglSurface==NULL){
			std::cout << "eglCreateWindowSurface failure" << std::endl;
			return 7;
		}

		EGLBoolean current=eglMakeCurrent(eglDisplay, eglSurface, eglSurface , eglContext);
		if(!current){
			std::cout << "eglMakeCurrent failure" << std::endl;
			return 8;
		}

		std::cout << "GL_VERSION " << glString(GL_VERSION) << std::endl;
		std::cout << "GL_SHADING_LANGUAGE_VERSION " << glString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
		std::cout << "GL_VENDOR " << glString(GL_VENDOR) << std::endl;
		std::cout << "GL_RENDERER " << glString(GL_RENDERER) << std::endl;
	//	std::cout << "GL_EXTENSIONS " << glString(GL_EXTENSIONS) << std::endl;
		return 0;
	}

	int flip(){
		EGLBoolean swap=eglSwapBuffers(eglDisplay, eglSurface);
		if(!swap){
			std::cout << "eglSwapBuffers failure" << std::endl;
			return 1;
		}
//		drmModeSettingSupported
		gbm_bo *bo = gbm_surface_lock_front_buffer(gbmSurface);
		if (!bo){
			std::cout<<"gbm_surface_lock_front_buffer fail"<<std::endl;
			return 2;
		}

		uint32_t fb=0;
		int addFBError=drmModeAddFB(fd, width,height, 24, 32, gbm_bo_get_stride(bo), gbm_bo_get_handle(bo).u32, &fb);	//32,32
		if (addFBError){
			std::cout << "drmModeAddFB fail error "<<addFBError<<std::endl;
			return 3;
		}

		// simon come here - replace with double buffer flipping

		int setCRTCError = drmModeSetCrtc(fd,crtcId,fb,0,0,&connectorIds[0],connectorIds.size(), &modeInfo);
		if (setCRTCError){
			std::cout << "drmModeSetCrtc fail error "<<setCRTCError<<std::endl;
			return 4;
		}

		if (prevFB){
			int rmFBError=drmModeRmFB(fd,prevFB);
			if (rmFBError){
				std::cout << "drmModeRmFB fail error " << rmFBError<<std::endl;
				return 5;
			}
		}
		prevFB = fb;

		if (prevBO){
			gbm_surface_release_buffer(gbmSurface, prevBO);
		}
		prevBO = bo;

		return 0;
	}

	int flop(){
		if (prevFB){
			int rmFBError=drmModeRmFB(fd,prevFB);
			if (rmFBError){
				std::cout << "drmModeRmFB fail error " << rmFBError<<std::endl;
				return 1;
			}
		}
		prevFB = 0;
		if (prevBO){
			gbm_surface_release_buffer(gbmSurface, prevBO);
		}
		prevBO=0;
		return 0;
	}
};
