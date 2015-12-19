#include "half.h"
#include <dlfcn.h>
#include "/usr/discreet/presets/2016/sparks/spark.h"
#include "openfx/include/ofxCore.h"

void *dlhandle;
int (*dlfunc)(void);

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
	dlhandle = dlopen("/Library/OFX/Plugins/NeatVideo4.ofx.bundle/Contents/MacOS/NeatVideo4.ofx", RTLD_LAZY);
	if(dlhandle == NULL) {
		printf("Ofxwrap: dlopen() failed\n");
		sparkError("Ofxwrap: failed to dlopen() OFX plugin!");
	}
	dlfunc = (int(*)()) dlsym(dlhandle, "OfxGetNumberOfPlugins");
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
	SparkMemBufStruct result, front;

	if(!bufferReady(1, &result)) return(NULL);
	if(!bufferReady(2, &front)) return(NULL);
	return NULL;
}

void SparkUnInitialise(SparkInfoStruct si) {
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
