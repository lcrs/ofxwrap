#include "half.h"
#include <dlfcn.h>
#include <unistd.h>
#include <stdarg.h>
#include "spark.h"
#include "openfx/include/ofxCore.h"
#include "openfx/include/ofxProperty.h"
#include "openfx/include/ofxImageEffect.h"
#include "openfx/include/ofxParam.h"
#include "openfx/include/ofxDialog.h"

void *dlhandle = NULL;
OfxHost h;
OfxPlugin *plugin = NULL;
OfxPropertySetHandle hostpropsethandle = (OfxPropertySetHandle) "hostpropset";
OfxImageEffectHandle imageeffecthandle = (OfxImageEffectHandle) "imageeffect";
OfxPropertySetHandle describeincontextpropsethandle = (OfxPropertySetHandle) "describeincontextprops";
OfxImageEffectHandle instancehandle = NULL;
OfxPropertySetHandle beginseqpropsethandle = (OfxPropertySetHandle) "beginseq";
OfxPropertySetHandle renderpropsethandle = (OfxPropertySetHandle) "render";
OfxPropertySetHandle begininstancechangepropsethandle = (OfxPropertySetHandle) "begininstancechangeprops";
OfxPropertySetHandle preparebuttonchangepropsethandle = (OfxPropertySetHandle) "preparebuttonchangeprops";
OfxPropertySetHandle adjustbuttonchangepropsethandle = (OfxPropertySetHandle) "adjustbuttonchangeprops";
OfxPropertySetHandle endinstancechangepropsethandle = (OfxPropertySetHandle) "endinstancechangeprops";
OfxImageClipHandle sourcecliphandle = (OfxImageClipHandle) "sourceclip";
OfxImageClipHandle outputcliphandle = (OfxImageClipHandle) "outputclip";
OfxPropertySetHandle sourceclippropsethandle = (OfxPropertySetHandle) "sourceclippropset";
OfxPropertySetHandle outputclippropsethandle = (OfxPropertySetHandle) "outputclippropset";
OfxPropertySetHandle currentframeimagehandle = (OfxPropertySetHandle) NULL;
OfxPropertySetHandle temporalframeimagehandles[11];
OfxPropertySetHandle outputimagehandle = (OfxPropertySetHandle) NULL;
OfxParamSetHandle describeincontextparams = (OfxParamSetHandle) "describeincontextparams";
OfxParamSetHandle instanceparams = (OfxParamSetHandle) "instanceparams";
OfxParamHandle dnp = (OfxParamHandle) "DNP";
OfxParamHandle nfp = (OfxParamHandle) "NFP";
OfxParamHandle adjustspatial = (OfxParamHandle) "Adjust Spatial...";
OfxParamHandle paramshash1 = (OfxParamHandle) "ParamsHash1";
OfxParamHandle paramshash2 = (OfxParamHandle) "ParamsHash2";
OfxParamHandle paramshash3 = (OfxParamHandle) "ParamsHash3";

void *instancedata = NULL;
SparkPixelFormat sparkdepth;
int sparkstride = 0;
double sparktime = 0.0;
int sparkw = 0;
int sparkh = 0;
long uniqueid = 0;
char *uniquestring = NULL;
int temporalids[11];

#include "props.h"
#include "dialogs.h"
#include "ifxs.h"
#include "parms.h"
#include "setups.h"

#ifdef __APPLE__
	#define PLUGIN "/Library/OFX/Plugins/NeatVideo4.ofx.bundle/Contents/MacOS/NeatVideo4.ofx"
#else
	// #define PLUGIN "/usr/OFX/Plugins/NeatVideo4.ofx.bundle/Contents/Linux-x86-64/NeatVideo4.ofx"
	#define PLUGIN "/usr/discreet/sparks/neat_unpacked/NeatVideo4.ofx.bundle/Contents/Linux-x86-64/NeatVideo4.ofx"
#endif

// UI controls page 1, controls 6-34
//	 6    13    20    27    34
//	 7    14    21    28
//	 8    15    22    29
//	 9    16    23    30
//	10    17    24    31
//	11    18    25    32
//	12    19    26    33
SparkStringStruct SparkString7 = {
	PLUGIN,
	(char *) "%s",
	SPARK_FLAG_NO_INPUT,
	NULL
};

unsigned long *prepare(int what, SparkInfoStruct si);
SparkPushStruct SparkPush9 = {
	(char *) "Prepare Noise Profile",
	prepare
};

unsigned long *adjust(int what, SparkInfoStruct si);
SparkPushStruct SparkPush10 = {
	(char *) "Adjust Filter Settings",
	adjust
};

const void *fetchSuite(OfxPropertySetHandle host, const char *suite, int version) {
	printf("Ofxwrap: fetchSuite() asked for suite %s version %d\n", suite, version);
	if(strcmp(suite, kOfxPropertySuite) == 0) {
		props_init();
		return (const void *)&props;
	}
	if(strcmp(suite, kOfxDialogSuite) == 0) {
		dialogs_init();
		return (const void *)&dialogs;
	}
	if(strcmp(suite, kOfxImageEffectSuite) == 0) {
		ifxs_init();
		return (const void *)&ifxs;
	}
	if(strcmp(suite, kOfxParameterSuite) == 0) {
		parms_init();
		return (const void *)&parms;
	}
	return NULL;
}

void die(const char *format, const char *arg) {
	char *m = (char *) malloc(100);
	sprintf(m, format, arg);
	printf("%s", m);
	sparkError(m);
	free(m);
}

void action(const char *a, OfxImageEffectHandle e, OfxPropertySetHandle in, OfxPropertySetHandle out) {
	if(plugin == NULL) {
		die("Ofxwrap: cannot do %s, plugin has gone away!\n", a);
	}
	OfxStatus s = plugin->mainEntry(a, e, in, out);
	switch(s) {
		case kOfxStatOK:
			printf("Ofxwrap: %s: ok\n", a);
			break;
		case kOfxStatReplyDefault:
			printf("Ofxwrap: %s: take default action\n", a);
			break;
		case kOfxStatFailed:
			die("Ofxwrap: %s: failed!\n", a);
			break;
		case kOfxStatErrFatal:
			die("Ofxwrap: %s: fatal error!\n", a);
			break;
		case kOfxStatErrMissingHostFeature:
			die("Ofxwrap: %s: missing feature!\n", a);
			break;
		default:
			printf("Ofxwrap: %s: returned %d\n", a, s);
	}
}

int bufferReady(int id, SparkMemBufStruct *b) {
	if(!sparkMemGetBuffer(id, b)) {
		printf("Ofxwrap: Failed to get buffer %d\n", id);
		return 0;
	}
	if(!(b->BufState & MEMBUF_LOCKED)) {
		printf("Ofxwrap: Failed to lock buffer %d\n", id);
		return 0;
	}
	return 1;
}

unsigned int SparkInitialise(SparkInfoStruct si) {
	printf("\n\n\nOfxwrap: in SparkInitialise(), name is %s\n", si.Name);

	uniquestring = (char *) malloc(100);

	void *d = dlopen(PLUGIN, RTLD_LAZY | RTLD_NOLOAD);
	if(d != NULL) {
		printf("Ofxwrap: plugin seems to be already loaded, will use existing handle\n");
		dlhandle = d;
	} else {
		dlhandle = dlopen(PLUGIN, RTLD_LAZY);
		if(dlhandle == NULL) {
			die("Ofxwrap: failed to dlopen() OFX plugin!\n", NULL);
			return 0;
		}
	}

	int (*OfxGetNumberOfPlugins)(void);
	OfxGetNumberOfPlugins = (int(*)(void)) dlsym(dlhandle, "OfxGetNumberOfPlugins");
	if(OfxGetNumberOfPlugins == NULL) {
		die("Ofxwrap: failed to find plugin's OfxGetNumberOfPlugins symbol!\n", NULL);
		return 0;
	}
	int numplugs = (*OfxGetNumberOfPlugins)();
	printf("Ofxwrap: found %d plugins\n", numplugs);

	OfxPlugin *(*OfxGetPlugin)(int nth);
	OfxGetPlugin = (OfxPlugin*(*)(int nth)) dlsym(dlhandle, "OfxGetPlugin");
	if(OfxGetPlugin == NULL) {
		die("Ofxwrap: failed to find plugin's OfxGetPlugin symbol!\n", NULL);
		return 0;
	}
	plugin = (*OfxGetPlugin)(0);
	if(plugin == NULL) {
		die("Ofxwrap: failed to get first plugin!\n", NULL);
		return 0;
	}
	printf("Ofxwrap: plugin id is %s\n", plugin->pluginIdentifier);

	h.host = hostpropsethandle;
	h.fetchSuite = &fetchSuite;
	plugin->setHost(&h);

	action(kOfxActionLoad, NULL, NULL, NULL);

	action(kOfxActionDescribe, imageeffecthandle, NULL, NULL);

	// Crashes inside the plugin binary unless we do this :( Could be looking for
	// a user prefs folder and getting confused by Flame's real/effective uid mismatch
	int realuid = getuid();
	int effectiveuid = geteuid();
	setuid(realuid);

	action(kOfxImageEffectActionDescribeInContext, imageeffecthandle, describeincontextpropsethandle, NULL);

	instancehandle = (OfxImageEffectHandle) "instance";
	action(kOfxActionCreateInstance, instancehandle, NULL, NULL);

	// Back to normal please
	setuid(effectiveuid);

	action(kOfxImageEffectActionGetClipPreferences, instancehandle, NULL, NULL);

	return(SPARK_MODULE);
}

int SparkClips(void) {
	return 1;
}

void SparkMemoryTempBuffers(void) {
	printf("Ofxwrap: in SparkMemoryTempBuffers()...\n");
	for(int i = 0; i < 11; i++) {
		printf("Ofxwrap: index %d was %d, ", i, temporalids[i]);
		temporalids[i] = sparkMemRegisterBuffer();
		printf("now %d\n", temporalids[i]);
	}
	printf("Ofxwrap: ...done with SparkMemoryTempBuffers()\n");
}

void rgb16fp_to_rgba32fp(char *in, int stride, int inc, float *out) {
	for(int x = 0; x < sparkw; x++) {
		for(int y = 0; y < sparkh; y++) {
			out[y * sparkw * 4 + x * 4 + 0] = *(half *)(in + stride * y + inc * x + 0);
			out[y * sparkw * 4 + x * 4 + 1] = *(half *)(in + stride * y + inc * x + 2);
			out[y * sparkw * 4 + x * 4 + 2] = *(half *)(in + stride * y + inc * x + 4);
			out[y * sparkw * 4 + x * 4 + 3] = 1.0;
		}
	}
}

void rgb16int_to_rgba32fp(char *in, int stride, int inc, float *out) {
	for(int x = 0; x < sparkw; x++) {
		for(int y = 0; y < sparkh; y++) {
			out[y * sparkw * 4 + x * 4 + 0] = *(unsigned short *)(in + stride * y + inc * x + 0) / 65535.0;
			out[y * sparkw * 4 + x * 4 + 1] = *(unsigned short *)(in + stride * y + inc * x + 2) / 65535.0;
			out[y * sparkw * 4 + x * 4 + 2] = *(unsigned short *)(in + stride * y + inc * x + 4) / 65535.0;
			out[y * sparkw * 4 + x * 4 + 3] = 1.0;
		}
	}
}

void rgb8int_to_rgba32fp(char *in, int stride, int inc, float *out) {
	for(int x = 0; x < sparkw; x++) {
		for(int y = 0; y < sparkh; y++) {
			out[y * sparkw * 4 + x * 4 + 0] = *(unsigned char *)(in + stride * y + inc * x + 0) / 255.0;
			out[y * sparkw * 4 + x * 4 + 1] = *(unsigned char *)(in + stride * y + inc * x + 1) / 255.0;
			out[y * sparkw * 4 + x * 4 + 2] = *(unsigned char *)(in + stride * y + inc * x + 2) / 255.0;
			out[y * sparkw * 4 + x * 4 + 3] = 1.0;
		}
	}
}

float clamp(float d, float min, float max) {
  const float t = d < min ? min : d;
  return t > max ? max : t;
}

unsigned long *SparkProcess(SparkInfoStruct si) {
	printf("Ofxwrap: in SparkProcess(), name is %s\n", si.Name);

	SparkMemBufStruct result, front, temporalbuffers[11];
	if(!bufferReady(1, &result)) return(NULL);
	if(!bufferReady(2, &front)) return(NULL);
	for(int i = 0; i < 11; i++) {
		if(!bufferReady(temporalids[i], &temporalbuffers[i])) return(NULL);
	}

	sparktime = si.FrameNo;

	// If the resolution changes, we're gonna have a bad time here...
	sparkw = front.BufWidth;
	sparkh = front.BufHeight;
	sparkdepth = (SparkPixelFormat) front.BufDepth;

	// Let's be very clear here about which frame is in which buffer
	sparkGetFrame(SPARK_FRONT_CLIP, sparktime - 5, temporalbuffers[0].Buffer);
	sparkGetFrame(SPARK_FRONT_CLIP, sparktime - 4, temporalbuffers[1].Buffer);
	sparkGetFrame(SPARK_FRONT_CLIP, sparktime - 3, temporalbuffers[2].Buffer);
	sparkGetFrame(SPARK_FRONT_CLIP, sparktime - 2, temporalbuffers[3].Buffer);
	sparkGetFrame(SPARK_FRONT_CLIP, sparktime - 1, temporalbuffers[4].Buffer);
	sparkGetFrame(SPARK_FRONT_CLIP, sparktime + 1, temporalbuffers[5].Buffer);
	sparkGetFrame(SPARK_FRONT_CLIP, sparktime + 2, temporalbuffers[6].Buffer);
	sparkGetFrame(SPARK_FRONT_CLIP, sparktime + 3, temporalbuffers[7].Buffer);
	sparkGetFrame(SPARK_FRONT_CLIP, sparktime + 4, temporalbuffers[8].Buffer);
	sparkGetFrame(SPARK_FRONT_CLIP, sparktime + 5, temporalbuffers[9].Buffer);
	sparkGetFrame(SPARK_FRONT_CLIP, sparktime + 6, temporalbuffers[10].Buffer);

	if(currentframeimagehandle == NULL) {
		currentframeimagehandle = (OfxPropertySetHandle) malloc(sparkw * sparkh * 4 * 4);
	}
	switch(sparkdepth) {
		case SPARKBUF_RGB_48_3x16_FP:
			rgb16fp_to_rgba32fp((char *)front.Buffer, front.Stride, front.Inc, (float *)currentframeimagehandle);
			sparkstride = sparkw * 4 * 4;
		break;
		case SPARKBUF_RGB_48_3x10:
		case SPARKBUF_RGB_48_3x12:
			rgb16int_to_rgba32fp((char *)front.Buffer, front.Stride, front.Inc, (float *)currentframeimagehandle);
			sparkstride = sparkw * 4 * 4;
		break;
		case SPARKBUF_RGB_24_3x8:
			rgb8int_to_rgba32fp((char *)front.Buffer, front.Stride, front.Inc, (float *)currentframeimagehandle);
			sparkstride = sparkw * 4 * 4;
		break;
		default:
			die("Ofxwrap: in SparkProcess(), unhandled pixel depth!\n", NULL);
	}
	printf("Ofxwrap: in SparkProcess(), currentframe is %p\n", currentframeimagehandle);
	printf("Ofxwrap: in SparkProcess(), front buffer is %p\n", front.Buffer);

	for(int i = 0; i < 11; i++) {
		OfxPropertySetHandle *h = &temporalframeimagehandles[i];
		SparkMemBufStruct *b = &temporalbuffers[i];
		if(*h == NULL) {
			*h = (OfxPropertySetHandle) malloc(sparkw * sparkh * 4 * 4);
		}
		switch(sparkdepth) {
			case SPARKBUF_RGB_48_3x16_FP:
				rgb16fp_to_rgba32fp((char *)b->Buffer, b->Stride, b->Inc, (float *)*h);
			break;
			case SPARKBUF_RGB_48_3x10:
			case SPARKBUF_RGB_48_3x12:
				rgb16int_to_rgba32fp((char *)b->Buffer, b->Stride, b->Inc, (float *)*h);
			break;
			case SPARKBUF_RGB_24_3x8:
				rgb8int_to_rgba32fp((char *)b->Buffer, b->Stride, b->Inc, (float *)*h);
			break;
			default:
				die("Ofxwrap: in SparkProcess(), unhandled pixel depth!\n", NULL);
		}
	}

	if(outputimagehandle == NULL) {
		outputimagehandle = (OfxPropertySetHandle) malloc(sparkw * sparkh * 4 * 4);
	}
	printf("Ofxwrap: in SparkProcess(), outputframe is %p\n", outputimagehandle);
	printf("Ofxwrap: in SparkProcess(), result buffer is %p\n", result.Buffer);

	action(kOfxImageEffectActionBeginSequenceRender, instancehandle, beginseqpropsethandle, NULL);
	action(kOfxImageEffectActionRender, instancehandle, renderpropsethandle, NULL);
	action(kOfxImageEffectActionEndSequenceRender, instancehandle, beginseqpropsethandle, NULL);

	char *rb = (char *) result.Buffer;
	float *oih = (float *) outputimagehandle;
	switch(sparkdepth) {
		case SPARKBUF_RGB_48_3x16_FP:
			for(int x = 0; x < sparkw; x++) {
				for(int y = 0; y < sparkh; y++) {
					*(half *)(rb + result.Stride * y + result.Inc * x + 0) = oih[y * sparkw * 4 + x * 4 + 0];
					*(half *)(rb + result.Stride * y + result.Inc * x + 2) = oih[y * sparkw * 4 + x * 4 + 1];
					*(half *)(rb + result.Stride * y + result.Inc * x + 4) = oih[y * sparkw * 4 + x * 4 + 2];
				}
			}
		break;
		case SPARKBUF_RGB_48_3x10:
		case SPARKBUF_RGB_48_3x12:
			for(int x = 0; x < sparkw; x++) {
				for(int y = 0; y < sparkh; y++) {
					*(unsigned short *)(rb + result.Stride * y + result.Inc * x + 0) = clamp(oih[y * sparkw * 4 + x * 4 + 0], 0, 1) * 65535.0;
					*(unsigned short *)(rb + result.Stride * y + result.Inc * x + 2) = clamp(oih[y * sparkw * 4 + x * 4 + 1], 0, 1) * 65535.0;
					*(unsigned short *)(rb + result.Stride * y + result.Inc * x + 4) = clamp(oih[y * sparkw * 4 + x * 4 + 2], 0, 1) * 65535.0;
				}
			}
		break;
		case SPARKBUF_RGB_24_3x8:
			for(int x = 0; x < sparkw; x++) {
				for(int y = 0; y < sparkh; y++) {
					*(unsigned char *)(rb + result.Stride * y + result.Inc * x + 0) = clamp(oih[y * sparkw * 4 + x * 4 + 0], 0, 1) * 255.0;
					*(unsigned char *)(rb + result.Stride * y + result.Inc * x + 1) = clamp(oih[y * sparkw * 4 + x * 4 + 1], 0, 1) * 255.0;
					*(unsigned char *)(rb + result.Stride * y + result.Inc * x + 2) = clamp(oih[y * sparkw * 4 + x * 4 + 2], 0, 1) * 255.0;
				}
			}
		break;
		default:
			die("Ofxwrap: in SparkProcess(), unhandled pixel depth!\n", NULL);
	}

	return(result.Buffer);
}

void SparkUnInitialise(SparkInfoStruct si) {
	printf("Ofxwrap: in SparkUnInitialise(), name is %s\n", si.Name);

	// Gonna have to just leak instances here
	// Calling this destroy action causes the plugin to try to access things
	// from other instances which may no longer exist, probably the host struct stuff
	// if(instancehandle != NULL) action(kOfxActionDestroyInstance, instancehandle, NULL, NULL);
	instancehandle = NULL;
	instancedata = NULL;
	free(uniquestring);
	uniquestring = NULL;
	free(currentframeimagehandle);
	currentframeimagehandle = NULL;
	free(outputimagehandle);
	outputimagehandle = NULL;
	for(int i = 0; i < 11; i++) {
		free(temporalframeimagehandles[i]);
		temporalframeimagehandles[i] = NULL;
	}

	// We can't do this because other instances of the Spark may be alive still
	// action(kOfxActionUnload, NULL, NULL, NULL);

	int r = dlclose(dlhandle);
	if(r == 0) {
		printf("Ofxwrap: dlclose() ok\n");
		dlhandle = NULL;
	} else {
		printf("Ofxwrap: dlclose() failed! Reason: %s\n", dlerror());
	}

	void *d = dlopen(PLUGIN, RTLD_LAZY | RTLD_NOLOAD);
	if(d != NULL) {
		printf("Ofxwrap: plugin semms to still be linked after dlclose()!\n");
	}
	dlclose(d);
}

int SparkIsInputFormatSupported(SparkPixelFormat fmt) {
	switch(fmt) {
		case SPARKBUF_RGB_24_3x8:
		case SPARKBUF_RGB_48_3x10:
		case SPARKBUF_RGB_48_3x12:
		case SPARKBUF_RGB_48_3x16_FP:
			return 1;
		break;
		default:
			printf("Ofxwrap: SparkIsInputFormatSupported(), unhandled pixel depth %d, failing!\n", fmt);
			return 0;
	}
}

void SparkEvent(SparkModuleEvent e) {
	printf("Ofxwrap: in SparkEvent(), event is %d, last setup name is %s\n", (int)e, sparkGetLastSetupName());
	if(e == SPARK_EVENT_RESULT) {
		sparkReprocess();
	}
}

void SparkSetupIOEvent(SparkModuleEvent e, char *path, char *file) {
	printf("Ofxwrap: in SparkIOEvent(), event is %d, path is %s, file is %s\n", (int)e, path, file);
	if(e == SPARK_EVENT_SAVESETUP) {
		if(instancehandle == NULL) return;
		action(kOfxActionSyncPrivateData, instancehandle, NULL, NULL);
		setups_save(path, file);
	} else if(e == SPARK_EVENT_LOADSETUP) {
		setups_load(path, file);
	}
}

unsigned long *prepare(int what, SparkInfoStruct si) {
	action(kOfxActionBeginInstanceChanged, instancehandle, begininstancechangepropsethandle, NULL);
	action(kOfxActionInstanceChanged, instancehandle, preparebuttonchangepropsethandle, NULL);
	action(kOfxActionEndInstanceChanged, instancehandle, endinstancechangepropsethandle, NULL);
	sparkReprocess();
	return NULL;
}

unsigned long *adjust(int what, SparkInfoStruct si) {
	action(kOfxActionBeginInstanceChanged, instancehandle, begininstancechangepropsethandle, NULL);
	action(kOfxActionInstanceChanged, instancehandle, adjustbuttonchangepropsethandle, NULL);
	action(kOfxActionEndInstanceChanged, instancehandle, endinstancechangepropsethandle, NULL);
	sparkReprocess();
	return NULL;
}
