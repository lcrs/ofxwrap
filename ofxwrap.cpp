#include "half.h"
#include <dlfcn.h>
#include <unistd.h>
#include "spark.h"
#include "openfx/include/ofxCore.h"
#include "openfx/include/ofxProperty.h"
#include "openfx/include/ofxImageEffect.h"
#include "openfx/include/ofxParam.h"
#include "openfx/include/ofxDialog.h"

OfxPlugin *plugin = NULL;
OfxPropertySetHandle hostpropsethandle = (OfxPropertySetHandle) "hostpropset";
OfxImageEffectHandle imageeffecthandle = (OfxImageEffectHandle) "imageeffect";
OfxPropertySetHandle describeincontextpropsethandle = (OfxPropertySetHandle) "describeincontextprops";
OfxImageEffectHandle instancehandle = (OfxImageEffectHandle) "instance";
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
OfxPropertySetHandle currentframeimagehandle = (OfxPropertySetHandle) "currentframeimage";
OfxPropertySetHandle outputimagehandle = (OfxPropertySetHandle) "outputimage";
void *instancedata = NULL;
double sparktime = 0.0;
int sparkw = 0;
int sparkh = 0;
float * __restrict__ currentframe = NULL;
float * __restrict__ outputframe = NULL;
long uniqueid = 0;
char *uniquestring = NULL;

#include "props.h"
#include "dialogs.h"
#include "ifxs.h"
#include "parms.h"

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
	sparkError(m);
	free(m);
}

void action(const char *a, OfxImageEffectHandle e, OfxPropertySetHandle in, OfxPropertySetHandle out) {
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
		case kOfxStatErrFatal:
			die("Ofxwrap: %s: fatal error!\n", a);
		case kOfxStatErrMissingHostFeature:
			die("Ofxwrap: %s: missing feature!\n", a);
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

	void *dlhandle = dlopen(PLUGIN, RTLD_LAZY);
	if(dlhandle == NULL) {
		die("Ofxwrap: failed to dlopen() OFX plugin!\n", NULL);
		return 0;
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

	OfxHost h;
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
	action(kOfxActionCreateInstance, instancehandle, NULL, NULL);

	// Back to normal please
	setuid(effectiveuid);

	action(kOfxImageEffectActionGetClipPreferences, instancehandle, NULL, NULL);

	return(SPARK_MODULE);
}

int SparkClips(void) {
	return 1;
}

unsigned long *SparkProcess(SparkInfoStruct si) {
	printf("Ofxwrap: in SparkProcess(), name is %s\n", si.Name);

	SparkMemBufStruct result, front;
	if(!bufferReady(1, &result)) return(NULL);
	if(!bufferReady(2, &front)) return(NULL);

	sparktime = si.FrameNo;
	sparkw = front.BufWidth;
	sparkh = front.BufHeight;

	// Convert RGB 16-bit half buffers to RGBA 32-bit float
	currentframe = (float *) malloc(sparkw * sparkh * 4 * 4);
	for(int x = 0; x < sparkw; x++) {
		for(int y = 0; y < sparkh; y++) {
			currentframe[y * sparkw * 4 + x * 4 + 0] = *((half *) (((char *)front.Buffer) + front.Stride * y + front.Inc * x + 0));
			currentframe[y * sparkw * 4 + x * 4 + 1] = *((half *) (((char *)front.Buffer) + front.Stride * y + front.Inc * x + 2));
			currentframe[y * sparkw * 4 + x * 4 + 2] = *((half *) (((char *)front.Buffer) + front.Stride * y + front.Inc * x + 4));
			currentframe[y * sparkw * 4 + x * 4 + 3] = 1.0;
		}
	}

	outputframe = (float *) malloc(sparkw * sparkh * 4 * 4);

	action(kOfxImageEffectActionBeginSequenceRender, instancehandle, beginseqpropsethandle, NULL);
	action(kOfxImageEffectActionRender, instancehandle, renderpropsethandle, NULL);
	action(kOfxImageEffectActionEndSequenceRender, instancehandle, beginseqpropsethandle, NULL);

	// Convert RGBA 32-bit float output buffer to RGB 16-bit half float
	void * __restrict__ rbuf;
	rbuf = result.Buffer;

	for(int x = 0; x < sparkw; x++) {
		for(int y = 0; y < sparkh; y++) {
			*((half *) (((char *)rbuf) + result.Stride * y + result.Inc * x + 0)) = outputframe[y * sparkw * 4 + x * 4 + 0];
			*((half *) (((char *)rbuf) + result.Stride * y + result.Inc * x + 2)) = outputframe[y * sparkw * 4 + x * 4 + 1];
			*((half *) (((char *)rbuf) + result.Stride * y + result.Inc * x + 4)) = outputframe[y * sparkw * 4 + x * 4 + 2];
		}
	}

	free(currentframe);
	free(outputframe);

	return(result.Buffer);
}

void SparkUnInitialise(SparkInfoStruct si) {
	printf("Ofxwrap: in SparkUnInitialise(), name is %s\n", si.Name);
	free(uniquestring);
}

void SparkMemoryTempBuffers(void) {
	// This has to be defined to keep Batch happy
}

int SparkIsInputFormatSupported(SparkPixelFormat fmt) {
	if(fmt == SPARKBUF_RGB_48_3x16_FP) {
		return 1;
	} else {
		return 0;
	}
}

void SparkEvent(SparkModuleEvent e) {
	printf("Ofxwrap: in SparkEvent(), event is %d, last setup name is %s\n", (int)e, sparkGetLastSetupName());
}

void SparkSetupIOEvent(SparkModuleEvent e, char *path, char *file) {
	printf("Ofxwrap: in SparkIOEvent(), event is %d, path is %s, file is %s\n", (int)e, path, file);
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
