#include "half.h"
#include <dlfcn.h>

#include "openfx/include/ofxCore.h"
#include "props.h"
#include "dialogs.h"
#include "ifxs.h"
#include "parms.h"

void sparkError(const char *s) {
	printf("%s", s);
}

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

int main(int argc, char **argv){
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
	OfxPlugin *plugin = NULL;
	plugin = (*OfxGetPlugin)(0);
	if(plugin == NULL) {
		sparkError("Ofxwrap: failed to get first plugin!\n");
	}
	printf("Ofxwrap: plugin id is %s\n", plugin->pluginIdentifier);

	OfxHost h;
	h.host = hostpropsethandle;
	h.fetchSuite = &fetchSuite;
	plugin->setHost(&h);
	OfxStatus s = plugin->mainEntry(kOfxActionLoad, NULL, NULL, NULL);
	switch(s) {
		case kOfxStatOK:
			printf("Ofxwrap: load action: ok\n");
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

	s = plugin->mainEntry(kOfxActionDescribe, imageeffect, NULL, NULL);
	switch(s) {
		case kOfxStatOK:
			printf("Ofxwrap: describe action: ok\n");
			break;
		case kOfxStatFailed:
			sparkError("Ofxwrap: describe action: failed!");
		case kOfxStatErrFatal:
			sparkError("Ofxwrap: describe action: fatal error!");
		case kOfxStatErrMissingHostFeature:
			sparkError("Ofxwrap: describe action: missing feature!");
		default:
			printf("Ofxwrap: describe action: returned %d\n", s);
	}

	const char *inprops = "describeincontextprops";
	OfxPropertySetHandle inpropshandle = (OfxPropertySetHandle) inprops;
	s = plugin->mainEntry(kOfxImageEffectActionDescribeInContext, imageeffect, inpropshandle, NULL);
	switch(s) {
		case kOfxStatOK:
			printf("Ofxwrap: describeincontext action: ok\n");
			break;
		case kOfxStatFailed:
			sparkError("Ofxwrap: describeincontext action: failed!");
		case kOfxStatErrFatal:
			sparkError("Ofxwrap: describeincontext action: fatal error!");
		case kOfxStatErrMissingHostFeature:
			sparkError("Ofxwrap: describeincontext action: missing feature!");
		default:
			printf("Ofxwrap: describeincontext action: returned %d\n", s);
	}

	s = plugin->mainEntry(kOfxActionCreateInstance, instancehandle, NULL, NULL);
	switch(s) {
		case kOfxStatOK:
			printf("Ofxwrap: create action: ok\n");
			break;
		case kOfxStatFailed:
			sparkError("Ofxwrap: create action: failed!");
		case kOfxStatErrFatal:
			sparkError("Ofxwrap: create action: fatal error!");
		case kOfxStatErrMissingHostFeature:
			sparkError("Ofxwrap: create action: missing feature!");
		default:
			printf("Ofxwrap: create action: returned %d\n", s);
	}

	return 1;
}
