#include "half.h"
#include <dlfcn.h>
#include "/usr/discreet/presets/2016/sparks/spark.h"
#include "openfx/include/ofxCore.h"

int bufferReady(int n, SparkMemBufStruct *b) {
	if(!sparkMemGetBuffer(n, b)) {
		printf("Ofxwrap: Failed to get buffer %d\n", n);
		return 0;
	}
	if(!(b->BufState & MEMBUF_LOCKED)) {
		printf("Ofxwrap: Failed to lock buffer %d\n", n);
		return 0;
	}
	return 1;
}

unsigned int SparkInitialise(SparkInfoStruct si) {
	printf("Ofxwrap: in SparkInitialise(), name is %s\n", si.Name);
	void *dlhandle = dlopen("/Library/OFX/Plugins/NeatVideo4.ofx.bundle/Contents/MacOS/NeatVideo4.ofx", RTLD_LAZY);
	if(dlhandle == NULL) {
		printf("Ofxwrap: dlopen() failed\n");
		sparkError("Ofxwrap: failed to dlopen() OFX plugin!");
	}
	int (*dlfunc)(void);
	dlfunc = (int(*)(void)) dlsym(dlhandle, "OfxGetNumberOfPlugins");
	if(dlfunc == NULL) {
		printf("Ofxwrap: dlsym() failed\n");
		sparkError("Ofxwrap: failed to find plugin entry function!\n");
	} else {
		int numplugs = (*dlfunc)();
		printf("Ofxwrap: found %d plugins\n", numplugs);
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
	;
}

void SparkSetupIOEvent(SparkModuleEvent e, char *path, char *file) {
	printf("Ofxwrap: in SparkIOEvent(), event is %d, path is %s, file is %s\n", (int)e, path, file);
}
