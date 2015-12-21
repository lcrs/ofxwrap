#include "half.h"
#include <dlfcn.h>
#include "/usr/discreet/presets/2016/sparks/spark.h"
#include "openfx/include/ofxCore.h"
#include "props.h"
#include "dialogs.h"
#include "ifxs.h"
#include "parms.h"

OfxPlugin *plugin = NULL;
OfxPropertySetHandle hostpropset = (OfxPropertySetStruct *) 0x1234;

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
	printf("Ofxwrap: in SparkInitialise(), name is %s\n", si.Name);

	void *dlhandle = dlopen("/Library/OFX/Plugins/NeatVideo4.ofx.bundle/Contents/MacOS/NeatVideo4.ofx", RTLD_LAZY);
	if(dlhandle == NULL) {
		sparkError("Ofxwrap: failed to dlopen() OFX plugin!");
	}

	int (*OfxGetNumberOfPlugins)(void);
	OfxGetNumberOfPlugins = (int(*)(void)) dlsym(dlhandle, "OfxGetNumberOfPlugins");
	if(OfxGetNumberOfPlugins == NULL) {
		sparkError("Ofxwrap: failed to find plugin's OfxGetNumberOfPlugins symbol!\n");
	}
	int numplugs = (*OfxGetNumberOfPlugins)();
	printf("Ofxwrap: found %d plugins\n", numplugs);

	OfxPlugin *(*OfxGetPlugin)(int nth);
	OfxGetPlugin = (OfxPlugin*(*)(int nth)) dlsym(dlhandle, "OfxGetPlugin");
	if(OfxGetPlugin == NULL) {
		sparkError("Ofxwrap: failed to find plugin's OfxGetPlugin symbol!\n");
	}
	plugin = (*OfxGetPlugin)(0);
	if(plugin == NULL) {
		sparkError("Ofxwrap: failed to get first plugin!\n");
	}
	printf("Ofxwrap: plugin id is %s\n", plugin->pluginIdentifier);

	OfxHost h;
	h.host = hostpropset;
	h.fetchSuite = &fetchSuite;
	plugin->setHost(&h);
	OfxStatus s = plugin->mainEntry(kOfxActionLoad, NULL, NULL, NULL);
	switch(s) {
		case kOfxStatOK:
			printf("Ofxwrap: load action: ok\n");
			break;
		case kOfxStatReplyDefault:
			printf("Ofxwrap: load action: ignored!\n");
			break;
		case kOfxStatFailed:
			sparkError("Ofxwrap: load action: failed!");
		case kOfxStatErrFatal:
			sparkError("Ofxwrap: load action: fatal error!");
		case kOfxStatErrMissingHostFeature:
			sparkError("Ofxwrap: load action: missing feature!");
		default:
			printf("Ofxwrap: load action: returned %d\n", s);
	}

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
	return NULL;
}

void SparkUnInitialise(SparkInfoStruct si) {
	printf("Ofxwrap: in SparkUnInitialise(), name is %s\n", si.Name);
}

void SparkMemoryTempBuffers(void) {
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
