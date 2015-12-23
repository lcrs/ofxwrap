// Setters
OfxStatus props_SetPointer(OfxPropertySetHandle properties, const char *property, int index, void *value) {
	printf("Ofxwrap: in props_SetPointer(), handle is %p, property is %s, index is %d, value is %p\n", properties, property, index, value);
	if(strcmp(property, kOfxPropInstanceData) == 0) {
		printf("Ofxwrap: in props_SetPointer(), setting instance data pointer to %p\n", value);
		instancedata = value;
	}
	return kOfxStatOK;
}
OfxStatus props_SetString(OfxPropertySetHandle properties, const char *property, int index, const char *value) {
	printf("Ofxwrap: in props_SetPointer(), handle is %p, property is %s, index is %d, value is %s\n", properties, property, index, value);
	return kOfxStatOK;
}
OfxStatus props_SetDouble(OfxPropertySetHandle properties, const char *property, int index, double value) {
	printf("Ofxwrap: in props_SetDouble(), handle is %p, property is %s, index is %d, value is %f\n", properties, property, index, value);
	return kOfxStatOK;
}
OfxStatus props_SetInt(OfxPropertySetHandle properties, const char *property, int index, int value) {
	printf("Ofxwrap: in props_SetInt(), handle is %p, property is %s, index is %d, value is %d\n", properties, property, index, value);
	return kOfxStatOK;
}

// Multi-dimensional setters
OfxStatus props_SetPointerN(OfxPropertySetHandle properties, const char *property, int count, void *const*value) {
	printf("Ofxwrap: in props_SetPointerN(), handle is %p, property is %s, count is %d, value is %p\n", properties, property, count, value);
	return kOfxStatOK;
}
OfxStatus props_SetStringN(OfxPropertySetHandle properties, const char *property, int count, const char *const*value) {
	printf("Ofxwrap: in props_SetStringN(), handle is %p, property is %s, count is %d, value is %p\n", properties, property, count, value);
	return kOfxStatOK;
}
OfxStatus props_SetDoubleN(OfxPropertySetHandle properties, const char *property, int count, const double *value) {
	printf("Ofxwrap: in props_SetDoubleN(), handle is %p, property is %s, count is %d, value is %p\n", properties, property, count, value);
	return kOfxStatOK;
}
OfxStatus props_SetIntN(OfxPropertySetHandle properties, const char *property, int count, const int *value) {
	printf("Ofxwrap: in props_SetIntN(), handle is %p, property is %s, count is %d, value is %p\n", properties, property, count, value);
	return kOfxStatOK;
}

// Getters
OfxStatus props_GetPointer(OfxPropertySetHandle properties, const char *property, int index, void **value) {
	printf("Ofxwrap: in props_GetPointer(), handle is %p, property is %s, index is %d, value is %p\n", properties, property, index, value);
	if(strcmp(property, kOfxPropInstanceData) == 0) {
		printf("Ofxwrap: in props_GetPointer(), returning instance data pointer %p\n", instancedata);
		*value = instancedata;
	}
	if(properties == currentframeimagehandle) {
		if(strcmp(property, kOfxImagePropData) == 0) {
			*value = currentframe;
			printf("Ofxwrap: in props_GetPointer(), asked for source current frame pointer, returning %p\n", currentframe);
		}
	}
	if(properties == outputimagehandle) {
		if(strcmp(property, kOfxImagePropData) == 0) {
			*value = outputframe;
			printf("Ofxwrap: in props_GetPointer(), asked for output frame pointer, returning %p\n", outputframe);
		}
	}
	return kOfxStatOK;
}
OfxStatus props_GetString(OfxPropertySetHandle properties, const char *property, int index, char **value) {
	printf("Ofxwrap: in props_GetString(), handle is %p, property is %s, index is %d, value is %p\n", properties, property, index, value);
	if(properties == hostpropsethandle) {
		if(strcmp(property, kOfxPropName) == 0) {
			*value = (char *) "Dustbuster";
		}
	}
	if(strcmp((char *)properties, "describeincontextprops") == 0) {
		if(strcmp(property, kOfxImageEffectPropContext) == 0) {
			*value = (char *) kOfxImageEffectContextFilter;
			printf("Ofxwrap: in props_GetString(), returned filter context\n");
		}
	}
	if(strcmp(property, kOfxImageEffectPropPixelDepth) == 0) {
		*value = (char *) kOfxBitDepthFloat;
		printf("Ofxwrap: in props_GetString(), asked for pixel depth, returned %s\n", *value);
	}
	return kOfxStatOK;
}
OfxStatus props_GetDouble(OfxPropertySetHandle properties, const char *property, int index, double *value) {
	printf("Ofxwrap: in props_GetDouble(), handle is %p, property is %s, index is %d, value is %p\n", properties, property, index, value);
	if(strcmp(property, kOfxPropTime) == 0) {
		printf("Ofxwrap: in props_GetDouble(), asked for time, returning %f\n", sparktime);
		*value = sparktime;
	}
	return kOfxStatOK;
}
OfxStatus props_GetInt(OfxPropertySetHandle properties, const char *property, int index, int *value) {
	printf("Ofxwrap: in props_GetInt(), handle is %p, property is %s, index is %d, value is %p\n", properties, property, index, value);
	if(properties == hostpropsethandle) {
		if(strcmp(property, kOfxImageEffectPropTemporalClipAccess) == 0) {
			*value = 1;
		}
	}
	if(properties == currentframeimagehandle) {
		if(strcmp(property, kOfxImagePropRowBytes) == 0) {
			*value = sparkw * 4 * 4;  // RGBA 32-bit float pixels
			printf("Ofxwrap: in props_GetInt(), asked for source current frame row stride, returning %d\n", *value);
		}
	}
	return kOfxStatOK;
}

// Multi-dimensional getters
OfxStatus props_GetPointerN(OfxPropertySetHandle properties, const char *property, int count, void **value) {
	printf("Ofxwrap: in props_GetPointerN(), handle is %p, property is %s, count is %d, value is %p\n", properties, property, count, value);
	return kOfxStatOK;
}
OfxStatus props_GetStringN(OfxPropertySetHandle properties, const char *property, int count, char **value) {
	printf("Ofxwrap: in props_GetStringN(), handle is %p, property is %s, count is %d, value is %p\n", properties, property, count, value);
	return kOfxStatOK;
}
OfxStatus props_GetDoubleN(OfxPropertySetHandle properties, const char *property, int count, double *value) {
	printf("Ofxwrap: in props_GetDoubleN(), handle is %p, property is %s, count is %d, value is %p\n", properties, property, count, value);
	return kOfxStatOK;
}
OfxStatus props_GetIntN(OfxPropertySetHandle properties, const char *property, int count, int *value) {
	printf("Ofxwrap: in props_GetIntN(), handle is %p, property is %s, count is %d, value is %p\n", properties, property, count, value);
	if(properties == hostpropsethandle) {
		if(strcmp(property, kOfxPropVersion) == 0) {
			*value = 1;
		}
	}
	if(properties == currentframeimagehandle) {
		printf("Ofxwrap: in props_GetIntN(), propset is current frame image\n");
		if(strcmp(property, kOfxImagePropBounds) == 0) {
			value[0] = 0;
			value[1] = 0;
			value[2] = sparkw;
			value[3] = sparkh;
			printf("Ofxwrap: in props_GetIntN(), asked for clip bounds, returning %d %d %d %d\n", value[0], value[1], value[2], value[3]);
		}
	}
	return kOfxStatOK;
}

// Others
OfxStatus props_Reset(OfxPropertySetHandle properties, const char *property) {
	printf("Ofxwrap: in props_Reset(), handle is %p, property is %s\n", properties, property);
	return kOfxStatOK;
}
OfxStatus props_GetDimension(OfxPropertySetHandle properties, const char *property, int *count) {
	printf("Ofxwrap: in props_GetDimension(), handle is %p, property is %s, count is %p\n", properties, property, count);
	return kOfxStatOK;
}

OfxPropertySuiteV1 props;

void props_init(void) {
	printf("Ofxwrap: in props_init()\n");
	props.propSetPointer = props_SetPointer;
	props.propSetString = props_SetString;
	props.propSetDouble = props_SetDouble;
	props.propSetInt = props_SetInt;
	props.propSetPointerN = props_SetPointerN;
	props.propSetStringN = props_SetStringN;
	props.propSetDoubleN = props_SetDoubleN;
	props.propSetIntN = props_SetIntN;
	props.propGetPointer = props_GetPointer;
	props.propGetString = props_GetString;
	props.propGetDouble = props_GetDouble;
	props.propGetInt = props_GetInt;
	props.propGetPointerN = props_GetPointerN;
	props.propGetStringN = props_GetStringN;
	props.propGetDoubleN = props_GetDoubleN;
	props.propGetIntN = props_GetIntN;
	props.propReset = props_Reset;
	props.propGetDimension = props_GetDimension;
}
