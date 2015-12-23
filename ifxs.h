#include "openfx/include/ofxImageEffect.h"

OfxImageClipHandle sourcecliphandle = (OfxImageClipHandle) "sourceclip";
OfxImageClipHandle outputcliphandle = (OfxImageClipHandle) "outputclip";
OfxPropertySetHandle sourceclippropsethandle = (OfxPropertySetHandle) "sourceclippropset";
OfxPropertySetHandle outputclippropsethandle = (OfxPropertySetHandle) "outputclippropset";


OfxStatus ifxs_getPropertySet(OfxImageEffectHandle imageEffect, OfxPropertySetHandle *propHandle) {
	printf("Ofxwrap: in ifxs_getPropertySet(), effect handle is %p, prop handle is %p\n", imageEffect, propHandle);
  if(imageEffect == imageeffecthandle) {
    printf("Ofxwrap: in ifxs_getPropertySet(), (above effect handle is the one passed thru the describe action)\n");
  }
  if(imageEffect == instancehandle) {
    printf("Ofxwrap: in ifxs_getPropertySet(), above effect handle is the instance\n");
  }
	return kOfxStatOK;
}
OfxStatus ifxs_getParamSet(OfxImageEffectHandle imageEffect, OfxParamSetHandle *paramSet) {
	printf("Ofxwrap: in ifxs_getParamSet(), effect handle is %p, param set handle is %p\n", imageEffect, paramSet);
	return kOfxStatOK;
}

OfxStatus ifxs_clipDefine(OfxImageEffectHandle imageEffect, const char *name, OfxPropertySetHandle *propertySet) {
	printf("Ofxwrap: in ifxs_clipDefine(), effect handle is %p, name is %s, prop set handle is %p\n", imageEffect, name, propertySet);
	return kOfxStatOK;
}

OfxStatus ifxs_clipGetHandle(OfxImageEffectHandle imageEffect, const char *name, OfxImageClipHandle *clip, OfxPropertySetHandle *propertySet) {
	printf("Ofxwrap: in ifxs_clipGetHandle(), effect handle is %p, name is %s, clip is %p, prop set handle is %p\n", imageEffect, name, clip, propertySet);
	if(strcmp(name, "Source") == 0) {
		printf("Ofxwrap: in ifxs_clipGetHandle(), asked for source, returning clip %p\n", sourcecliphandle);
		*clip = sourcecliphandle;
	}
	if(strcmp(name, "Output") == 0) {
		printf("Ofxwrap: in ifxs_clipGetHandle(), asked for output, returning clip %p\n", outputcliphandle);
		*clip = outputcliphandle;
	}
	return kOfxStatOK;
}

OfxStatus ifxs_clipGetPropertySet(OfxImageClipHandle clip, OfxPropertySetHandle *propHandle) {
	printf("Ofxwrap: in ifxs_clipGetPropertySet(), clip is %p, prop handle is %p\n", clip, propHandle);
	return kOfxStatOK;
}

OfxStatus ifxs_clipGetImage(OfxImageClipHandle clip, OfxTime time, const OfxRectD *region, OfxPropertySetHandle *imageHandle) {
	printf("Ofxwrap: in ifxs_clipGetImage(), clip is %p, time is %f, region is %p, image handle is %p\n", clip, time, region, imageHandle);
	if(clip == sourcecliphandle) {
		printf("Ofxwrap: in ifxs_clipGetImage(), clip is source, returning... what, exactly?\n");
	}
	if(clip == outputcliphandle) {
		printf("Ofxwrap: in ifxs_clipGetImage(), clip is output, returning... a mystery\n");
	}
	return kOfxStatOK;
}

OfxStatus ifxs_clipReleaseImage(OfxPropertySetHandle imageHandle) {
	printf("Ofxwrap: in ifxs_clipReleaseImage(), image handle is %p\n", imageHandle);
	return kOfxStatOK;
}

OfxStatus ifxs_clipGetRegionOfDefinition(OfxImageClipHandle clip, OfxTime time, OfxRectD *bounds) {
	printf("Ofxwrap: in ifxs_clipGetRegionOfDefinition(), clip is %p, time is %f, bounds is %p\n", clip, time, bounds);
	return kOfxStatOK;
}

int ifxs_abort(OfxImageEffectHandle imageEffect) {
  printf("Ofxwrap: in ifxs_abort(), effect handle is %p\n", imageEffect);
  return kOfxStatOK;
}

OfxStatus ifxs_imageMemoryAlloc(OfxImageEffectHandle instanceHandle, size_t nBytes, OfxImageMemoryHandle *memoryHandle) {
	printf("Ofxwrap: in ifxs_imageMemoryAlloc(), effect handle is %p, size %lu, mem handle is %p\n", instanceHandle, nBytes, memoryHandle);
	return kOfxStatOK;
}

OfxStatus ifxs_imageMemoryFree(OfxImageMemoryHandle memoryHandle) {
	printf("Ofxwrap: in ifxs_imageMemoryFree(), mem handle is %p\n", memoryHandle);
	return kOfxStatOK;
}

OfxStatus ifxs_imageMemoryLock(OfxImageMemoryHandle memoryHandle, void **returnedPtr) {
	printf("Ofxwrap: in ifxs_imageMemoryLock(), mem handle is %p, ptr is %p\n", memoryHandle, returnedPtr);
	return kOfxStatOK;
}

OfxStatus ifxs_imageMemoryUnlock(OfxImageMemoryHandle memoryHandle) {
	printf("Ofxwrap: in ifxs_imageMemoryUnlock(), mem handle is %p\n", memoryHandle);
	return kOfxStatOK;
}

OfxImageEffectSuiteV1 ifxs;

void ifxs_init(void) {
  printf("Ofxwrap: in ifxs_init()\n");
  ifxs.getPropertySet = ifxs_getPropertySet;
  ifxs.getParamSet = ifxs_getParamSet;
  ifxs.clipDefine = ifxs_clipDefine;
  ifxs.clipGetHandle = ifxs_clipGetHandle;
  ifxs.clipGetPropertySet = ifxs_clipGetPropertySet;
  ifxs.clipGetImage = ifxs_clipGetImage;
  ifxs.clipReleaseImage = ifxs_clipReleaseImage;
  ifxs.clipGetRegionOfDefinition = ifxs_clipGetRegionOfDefinition;
  ifxs.abort = ifxs_abort;
  ifxs.imageMemoryAlloc = ifxs_imageMemoryAlloc;
  ifxs.imageMemoryFree = ifxs_imageMemoryFree;
  ifxs.imageMemoryLock = ifxs_imageMemoryLock;
  ifxs.imageMemoryUnlock = ifxs_imageMemoryUnlock;
}
