OfxStatus ifxs_getPropertySet(OfxImageEffectHandle imageEffect, OfxPropertySetHandle *propHandle) {
	say("Ofxwrap: in ifxs_getPropertySet(), effect handle is %p, prop handle is %p\n", imageEffect, propHandle);
  if(imageEffect == imageeffecthandle) {
    say("Ofxwrap: in ifxs_getPropertySet(), returning describer props\n");
		*propHandle = describerpropset;
		return kOfxStatOK;
  }
  if(imageEffect == instancehandle) {
    say("Ofxwrap: in ifxs_getPropertySet(), returning instance props\n");
		*propHandle = instancepropset;
		return kOfxStatOK;
  }
	say("Ofxwrap: in ifxs_getPropertySet(), did not handle!\n");
	return kOfxStatFailed;
}
OfxStatus ifxs_getParamSet(OfxImageEffectHandle imageEffect, OfxParamSetHandle *paramSet) {
	say("Ofxwrap: in ifxs_getParamSet(), effect handle is %p, param set handle is %p\n", imageEffect, paramSet);
	if(imageEffect == instancehandle) {
		*paramSet = instanceparams;
		say("Ofxwrap: in ifxs_getParamSet(), effect is the instance, returning params %p\n", *paramSet);
		return kOfxStatOK;
	} else if(imageEffect == imageeffecthandle) {
		*paramSet = describeincontextparams;
		say("Ofxwrap: in ifxs_getParamSet(), effect is the describer, returning params %p\n", *paramSet);
		return kOfxStatOK;
	} else {
		say("Ofxwrap: in ifxs_getParamSet(), asked about an effect handle we don't know!\n");
		return kOfxStatFailed;
	}
}

OfxStatus ifxs_clipDefine(OfxImageEffectHandle imageEffect, const char *name, OfxPropertySetHandle *propertySet) {
	say("Ofxwrap: in ifxs_clipDefine(), effect handle is %p, name is %s, prop set handle is %p\n", imageEffect, name, propertySet);
	return kOfxStatOK;
}

OfxStatus ifxs_clipGetHandle(OfxImageEffectHandle imageEffect, const char *name, OfxImageClipHandle *clip, OfxPropertySetHandle *propertySet) {
	say("Ofxwrap: in ifxs_clipGetHandle(), effect handle is %p, name is %s, clip is %p, prop set handle is %p\n", imageEffect, name, clip, propertySet);
	if(strcmp(name, "Source") == 0) {
		say("Ofxwrap: in ifxs_clipGetHandle(), asked for source, returning clip %p\n", sourcecliphandle);
		*clip = sourcecliphandle;
	}
	if(strcmp(name, "Output") == 0) {
		say("Ofxwrap: in ifxs_clipGetHandle(), asked for output, returning clip %p\n", outputcliphandle);
		*clip = outputcliphandle;
	}
	return kOfxStatOK;
}

OfxStatus ifxs_clipGetPropertySet(OfxImageClipHandle clip, OfxPropertySetHandle *propHandle) {
	say("Ofxwrap: in ifxs_clipGetPropertySet(), clip is %p, prop handle is %p\n", clip, propHandle);
	return kOfxStatOK;
}

OfxStatus ifxs_clipGetImage(OfxImageClipHandle clip, OfxTime time, const OfxRectD *region, OfxPropertySetHandle *imageHandle) {
	say("Ofxwrap: in ifxs_clipGetImage(), clip is %p, time is %f, region is %p, image handle is %p\n", clip, time, region, imageHandle);
	if(clip == sourcecliphandle) {
		say("Ofxwrap: in ifxs_clipGetImage(), clip is source, ");
		if(time == sparktime) {
			*imageHandle = currentframeimagehandle;
			say("frame is current, returning handle %p\n", *imageHandle);
			return kOfxStatOK;
		} else {
			// Again being ultra-explicit here...
			int delta = time - sparktime;
			switch(delta) {
				case -5:
					*imageHandle = temporalframeimagehandles[0];
				break;
				case -4:
					*imageHandle = temporalframeimagehandles[1];
				break;
				case -3:
					*imageHandle = temporalframeimagehandles[2];
				break;
				case -2:
					*imageHandle = temporalframeimagehandles[3];
				break;
				case -1:
					*imageHandle = temporalframeimagehandles[4];
				break;
				case 1:
					*imageHandle = temporalframeimagehandles[5];
				break;
				case 2:
					*imageHandle = temporalframeimagehandles[6];
				break;
				case 3:
					*imageHandle = temporalframeimagehandles[7];
				break;
				case 4:
					*imageHandle = temporalframeimagehandles[8];
				break;
				case 5:
					*imageHandle = temporalframeimagehandles[9];
				break;
				case 6:
					*imageHandle = temporalframeimagehandles[10];
				break;
				default:
					*imageHandle = currentframeimagehandle;
					say("frame is beyond what we were expecting! Returning current... ");
			}
			say("frame delta is %d, returning handle %p\n", delta, *imageHandle);
			return kOfxStatOK;
		}
	}
	if(clip == outputcliphandle) {
		say("Ofxwrap: in ifxs_clipGetImage(), clip is output, ");
		if(time == sparktime) {
			*imageHandle = outputimagehandle;
			say("frame is current, returning handle %p\n", *imageHandle);
			return kOfxStatOK;
		} else {
			*imageHandle = NULL;
			say("frame is not current, returning null and failure status!\n");
			return kOfxStatFailed;
		}
	}
	say("Ofxwrap: in ifxs_clipGetImage(), did not handle, returning failure!\n");
	return kOfxStatFailed;
}

OfxStatus ifxs_clipReleaseImage(OfxPropertySetHandle imageHandle) {
	say("Ofxwrap: in ifxs_clipReleaseImage(), image handle is %p\n", imageHandle);
	return kOfxStatOK;
}

OfxStatus ifxs_clipGetRegionOfDefinition(OfxImageClipHandle clip, OfxTime time, OfxRectD *bounds) {
	say("Ofxwrap: in ifxs_clipGetRegionOfDefinition(), clip is %p, time is %f, bounds is %p\n", clip, time, bounds);
	say("Ofxwrap: in ifxs_clipGetRegionOfDefinition(), did not handle, returning failure!\n");
	return kOfxStatFailed;
}

int ifxs_abort(OfxImageEffectHandle imageEffect) {
  say("Ofxwrap: in ifxs_abort(), effect handle is %p\n", imageEffect);
  return kOfxStatOK;
}

OfxStatus ifxs_imageMemoryAlloc(OfxImageEffectHandle instanceHandle, size_t nBytes, OfxImageMemoryHandle *memoryHandle) {
	say("Ofxwrap: in ifxs_imageMemoryAlloc(), effect handle is %p, size %lu, mem handle is %p\n", instanceHandle, nBytes, memoryHandle);
	return kOfxStatOK;
}

OfxStatus ifxs_imageMemoryFree(OfxImageMemoryHandle memoryHandle) {
	say("Ofxwrap: in ifxs_imageMemoryFree(), mem handle is %p\n", memoryHandle);
	return kOfxStatOK;
}

OfxStatus ifxs_imageMemoryLock(OfxImageMemoryHandle memoryHandle, void **returnedPtr) {
	say("Ofxwrap: in ifxs_imageMemoryLock(), mem handle is %p, ptr is %p\n", memoryHandle, returnedPtr);
	return kOfxStatOK;
}

OfxStatus ifxs_imageMemoryUnlock(OfxImageMemoryHandle memoryHandle) {
	say("Ofxwrap: in ifxs_imageMemoryUnlock(), mem handle is %p\n", memoryHandle);
	return kOfxStatOK;
}

OfxImageEffectSuiteV1 ifxs;

void ifxs_init(void) {
  say("Ofxwrap: in ifxs_init()\n");
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
