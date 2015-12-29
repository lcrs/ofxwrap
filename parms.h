OfxStatus parms_Define(OfxParamSetHandle paramSet, const char *paramType, const char *name, OfxPropertySetHandle *propertySet) {
  printf("Ofxwrap: in parms_Define(), parm set handle is %p, parm type is %s, name is %s, prop set handle is %p\n", paramSet, paramType, name, propertySet);
	return kOfxStatOK;
}

OfxStatus parms_GetHandle(OfxParamSetHandle paramSet, const char *name, OfxParamHandle *param, OfxPropertySetHandle *propertySet) {
  printf("Ofxwrap: in parms_GetHandle(), parm set handle is %p, name is %s, parm handle is %p, prop set handle is %p\n", paramSet, name, param, propertySet);
  if(strcmp(name, "DNP") == 0) {
    *param = dnp;
    printf("Ofxwrap: in parms_GetHandle(): returning DNP param handle %p\n", *param);
  	return kOfxStatOK;
  } else if(strcmp(name, "NFP") == 0) {
    *param = nfp;
    printf("Ofxwrap: in parms_GetHandle(): returning NFP param handle %p\n", *param);
  	return kOfxStatOK;
  } else if(strcmp(name, "Adjust Spatial...") == 0) {
    *param = adjustspatial;
    printf("Ofxwrap: in parms_GetHandle(): returning adjustspatial param handle %p\n", *param);
  	return kOfxStatOK;
  } else if(strcmp(name, "ParamsHash1") == 0) {
    *param = paramshash1;
    printf("Ofxwrap: in parms_GetHandle(): returning ParamsHash1 param handle %p\n", *param);
  	return kOfxStatOK;
  } else if(strcmp(name, "ParamsHash2") == 0) {
    *param = paramshash2;
    printf("Ofxwrap: in parms_GetHandle(): returning ParamsHash2 param handle %p\n", *param);
  	return kOfxStatOK;
  } else if(strcmp(name, "ParamsHash3") == 0) {
    *param = paramshash3;
    printf("Ofxwrap: in parms_GetHandle(): returning ParamsHash3 param handle %p\n", *param);
  	return kOfxStatOK;
  } else {
    printf("Ofxwrap: in parms_GetHandle(): unhandled param! %s\n", name);
    return kOfxStatFailed;
  }
}

OfxStatus parms_SetGetPropertySet(OfxParamSetHandle paramSet, OfxPropertySetHandle *propHandle) {
  printf("Ofxwrap: in parms_SetGetPropertySet(), parm set handle is %p, prop set handle is %p\n", paramSet, propHandle);
	return kOfxStatOK;
}

OfxStatus parms_GetPropertySet(OfxParamHandle param, OfxPropertySetHandle *propHandle) {
  printf("Ofxwrap: in parms_GetPropertySet(), parm handle is %p, prop set handle is %p\n", param, propHandle);
  if(param == adjustspatial) {
    printf("Ofxwrap: in parms_GetPropertySet(), returning adjustspatial props\n");
    *propHandle = adjustspatialprops;
    return kOfxStatOK;
  }
	return kOfxStatFailed;
}

OfxStatus parms_GetValue(OfxParamHandle paramHandle, ...) {
  printf("Ofxwrap: in parms_GetValue(), parm handle is %p\n", paramHandle);

  va_list ap;
  va_start(ap, paramHandle);
  if(paramHandle == dnp) {
    char **s = va_arg(ap, char **);
    printf("Ofxwrap: in parms_GetValue(), parm handle is dnp, arg is %p\n", s);
    if(dnp_data == NULL) {
      *s = strdup("");
    } else {
      *s = strdup(dnp_data);
    }
    return kOfxStatOK;
  } else if(paramHandle == nfp) {
    char **s = va_arg(ap, char **);
    printf("Ofxwrap: in parms_GetValue(), parm handle is nfp, arg is %p\n", s);
    if(nfp_data == NULL) {
      *s = strdup("");
    } else {
      *s = strdup(nfp_data);
    }
    return kOfxStatOK;
  } else if(paramHandle == paramshash1) {
    int *s = va_arg(ap, int *);
    printf("Ofxwrap: in parms_GetValue(), parm handle is paramshash1, arg is %p\n", s);
    *s = paramshash1_data;
    return kOfxStatOK;
  } else if(paramHandle == paramshash2) {
    int *s = va_arg(ap, int *);
    *s = paramshash2_data;
    return kOfxStatOK;
    printf("Ofxwrap: in parms_GetValue(), parm handle is paramshash2, arg is %p\n", s);
  } else if(paramHandle == paramshash3) {
    int *s = va_arg(ap, int *);
    *s = paramshash3_data;
    return kOfxStatOK;
    printf("Ofxwrap: in parms_GetValue(), parm handle is paramshash3, arg is %p\n", s);
  } else {
    printf("Ofxwrap: in parms_GetValue(), did not handle!\n");
    return kOfxStatFailed;
  }
  va_end(ap);
}

OfxStatus parms_GetValueAtTime(OfxParamHandle paramHandle, OfxTime time, ...) {
  printf("Ofxwrap: in parms_GetValueAtTime(), parm handle is %p, time is %f\n", paramHandle, time);
	return kOfxStatOK;
}

OfxStatus parms_GetDerivative(OfxParamHandle paramHandle, OfxTime time, ...) {
  printf("Ofxwrap: in parms_GetDerivative(), parm handle is %p, time is %f\n", paramHandle, time);
	return kOfxStatOK;
}

OfxStatus parms_GetIntegral(OfxParamHandle paramHandle, OfxTime time1, OfxTime time2, ...) {
  printf("Ofxwrap: in parms_GetIntegral(), parm handle is %p, time is %f to %f\n", paramHandle, time1, time2);
	return kOfxStatOK;
}

OfxStatus parms_SetValue(OfxParamHandle paramHandle, ...) {
  printf("Ofxwrap: in parms_SetValue(), parm handle is %p\n", paramHandle);

  va_list ap;
  va_start(ap, paramHandle);
  if(paramHandle == dnp) {
    char *s = va_arg(ap, char *);
    printf("Ofxwrap: in parms_SetValue(), parm handle is dnp, arg is %s\n", s);
    dnp_data = (char *) realloc(dnp_data, strlen(s));
    strcpy(dnp_data, s);
  } else if(paramHandle == nfp) {
    char *s = va_arg(ap, char *);
    printf("Ofxwrap: in parms_SetValue(), parm handle is nfp, arg is %s\n", s);
    nfp_data = (char *) realloc(nfp_data, strlen(s));
    strcpy(nfp_data, s);
  } else if(paramHandle == paramshash1) {
    int s = va_arg(ap, int);
    printf("Ofxwrap: in parms_SetValue(), parm handle is paramshash1, arg is %d\n", s);
    paramshash1_data = s;
  } else if(paramHandle == paramshash2) {
    int s = va_arg(ap, int);
    paramshash2_data = s;
    printf("Ofxwrap: in parms_SetValue(), parm handle is paramshash2, arg is %d\n", s);
  } else if(paramHandle == paramshash3) {
    int s = va_arg(ap, int);
    paramshash3_data = s;
    printf("Ofxwrap: in parms_SetValue(), parm handle is paramshash3, arg is %d\n", s);
  }
  va_end(ap);

	return kOfxStatOK;
}

OfxStatus parms_SetValueAtTime(OfxParamHandle paramHandle, OfxTime time, ...) {
  printf("Ofxwrap: in parms_SetValueAtTime(), parm handle is %p, time is %f\n", paramHandle, time);
	return kOfxStatOK;
}

OfxStatus parms_GetNumKeys(OfxParamHandle paramHandle, unsigned int *numberOfKeys) {
  printf("Ofxwrap: in parms_GetNumKeys(), parm handle is %p, number is %p\n", paramHandle, numberOfKeys);
	return kOfxStatOK;
}

OfxStatus parms_GetKeyTime(OfxParamHandle paramHandle, unsigned int nthKey, OfxTime *time) {
  printf("Ofxwrap: in parms_GetKeyTime(), parm handle is %p, key is %d, time is %p\n", paramHandle, nthKey, time);
	return kOfxStatOK;
}

OfxStatus parms_GetKeyIndex(OfxParamHandle paramHandle, OfxTime time, int direction, int *index) {
  printf("Ofxwrap: in parms_GetKeyIndex(), parm handle is %p, time is %f, direction is %d, index is %p\n", paramHandle, time, direction, index);
	return kOfxStatOK;
}

OfxStatus parms_DeleteKey(OfxParamHandle paramHandle, OfxTime time) {
  printf("Ofxwrap: in parms_DeleteKey(), parm handle is %p, time is %f\n", paramHandle, time);
	return kOfxStatOK;
}

OfxStatus parms_DeleteAllKeys(OfxParamHandle paramHandle) {
  printf("Ofxwrap: in parms_DeleteAllKeys(), parm handle is %p\n", paramHandle);
	return kOfxStatOK;
}

OfxStatus parms_Copy(OfxParamHandle paramTo, OfxParamHandle paramFrom, OfxTime dstOffset, const OfxRangeD *frameRange) {
  printf("Ofxwrap: in parms_Copy(), parm to handle is %p, parm from handle is %p, dstOffset is %f, framerange is %p\n", paramTo, paramFrom, dstOffset, frameRange);
	return kOfxStatOK;
}

OfxStatus parms_EditBegin(OfxParamSetHandle paramSet, const char *name) {
  printf("Ofxwrap: in parms_EditBegin(), parm set handle is %p, name is %s\n", paramSet, name);
	return kOfxStatOK;
}

OfxStatus parms_EditEnd(OfxParamSetHandle paramSet) {
  printf("Ofxwrap: in parms_EditEnd(), parm set handle is %p\n", paramSet);
	return kOfxStatOK;
}

OfxParameterSuiteV1 parms;

void parms_init(void) {
  printf("Ofxwrap: in parms_init()\n");
  parms.paramDefine = parms_Define;
  parms.paramGetHandle = parms_GetHandle;
  parms.paramSetGetPropertySet = parms_SetGetPropertySet;
  parms.paramGetPropertySet = parms_GetPropertySet;
  parms.paramGetValue = parms_GetValue;
  parms.paramGetValueAtTime = parms_GetValueAtTime;
  parms.paramGetDerivative = parms_GetDerivative;
  parms.paramGetIntegral = parms_GetIntegral;
  parms.paramSetValue = parms_SetValue;
  parms.paramSetValueAtTime = parms_SetValueAtTime;
  parms.paramGetNumKeys = parms_GetNumKeys;
  parms.paramGetKeyTime = parms_GetKeyTime;
  parms.paramGetKeyIndex = parms_GetKeyIndex;
  parms.paramDeleteKey = parms_DeleteKey;
  parms.paramDeleteAllKeys = parms_DeleteAllKeys;
  parms.paramCopy = parms_Copy;
  parms.paramEditBegin = parms_EditBegin;
  parms.paramEditEnd = parms_EditEnd;
}
