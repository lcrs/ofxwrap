#include "half.h"
#include <dlfcn.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include "spark.h"
#include "openfx/include/ofxCore.h"
#include "openfx/include/ofxProperty.h"
#include "openfx/include/ofxImageEffect.h"
#include "openfx/include/ofxParam.h"
#include "openfx/include/ofxDialog.h"

// We have a lot of global state
// Note that even though we are a shared library, this state is unique
// for each Spark "node" because Flame copies our binary, gives it a unique name
// then loads that copy for each new node or timeline soft effect
void *dlhandle = NULL;
OfxHost h;
OfxPlugin *plugin = NULL;

// These OFX handles are all "blind" - they're just arbitrary pointers passed back and forth
// across the API, they are never de-referenced to an actual types or memory locations
// We're using strings purely for clarity, and NULL to indicate something's not ready

// Properites of the host
OfxPropertySetHandle hostpropsethandle = (OfxPropertySetHandle) "hostpropset";

// Handle of the abstract image effect, used in the "describeincontext" action
// This abstract instance just holds properties describing how a real
// instance will behave, it's also referred to as the "describer" instance below
OfxImageEffectHandle imageeffecthandle = (OfxImageEffectHandle) "imageeffect";

// Handle of the actual concrete instance which is able to process pixels
OfxImageEffectHandle instancehandle = NULL;

// Propeties on the abstract describer
OfxPropertySetHandle describerpropset = (OfxPropertySetHandle) "describerpropset";

// Properties on the real working plugin instance
OfxPropertySetHandle instancepropset = (OfxPropertySetHandle) "instancepropset";

// The property sets are passed alongside action calls, to hold lists of options and return
// values for that action essentially
OfxPropertySetHandle describeincontextpropsethandle = (OfxPropertySetHandle) "describeincontextprops";
OfxPropertySetHandle beginseqpropsethandle = (OfxPropertySetHandle) "beginseq";
OfxPropertySetHandle renderpropsethandle = (OfxPropertySetHandle) "render";
OfxPropertySetHandle begininstancechangepropsethandle = (OfxPropertySetHandle) "begininstancechangeprops";
OfxPropertySetHandle preparebuttonchangepropsethandle = (OfxPropertySetHandle) "preparebuttonchangeprops";
OfxPropertySetHandle adjustbuttonchangepropsethandle = (OfxPropertySetHandle) "adjustbuttonchangeprops";
OfxPropertySetHandle dnpchangepropsethandle = (OfxPropertySetHandle) "dnpchangeprops";
OfxPropertySetHandle nfpchangepropsethandle = (OfxPropertySetHandle) "nfpchangeprops";
OfxPropertySetHandle paramshash1changepropsethandle = (OfxPropertySetHandle) "paramshash1changeprops";
OfxPropertySetHandle paramshash2changepropsethandle = (OfxPropertySetHandle) "paramshash2changeprops";
OfxPropertySetHandle paramshash3changepropsethandle = (OfxPropertySetHandle) "paramshash3changeprops";
OfxPropertySetHandle endinstancechangepropsethandle = (OfxPropertySetHandle) "endinstancechangeprops";

// Input and output clip handles correspond to Front and Result Spark clips
OfxImageClipHandle sourcecliphandle = (OfxImageClipHandle) "sourceclip";
OfxImageClipHandle outputcliphandle = (OfxImageClipHandle) "outputclip";
OfxPropertySetHandle sourceclippropsethandle = (OfxPropertySetHandle) "sourceclippropset";
OfxPropertySetHandle outputclippropsethandle = (OfxPropertySetHandle) "outputclippropset";

// Properties of source, output and 11 temporal input images
OfxPropertySetHandle currentframeimagehandle = (OfxPropertySetHandle) NULL;
OfxPropertySetHandle outputimagehandle = (OfxPropertySetHandle) NULL;
OfxPropertySetHandle temporalframeimagehandles[11];

// Handles for parameter sets, which are the UI controls defined by the OFX plugin,
// both abstract at the describing stage and real live parameters of the instance
OfxParamSetHandle describeincontextparams = (OfxParamSetHandle) "describeincontextparams";
OfxParamSetHandle instanceparams = (OfxParamSetHandle) "instanceparams";

// Parameters defined by the plugin, or the ones we're interested in at least
OfxParamHandle dnp = (OfxParamHandle) "DNP";
OfxParamHandle nfp = (OfxParamHandle) "NFP";
OfxParamHandle adjustspatial = (OfxParamHandle) "Adjust Spatial...";
OfxPropertySetHandle adjustspatialprops = (OfxPropertySetHandle) "adjustspatialprops";
OfxParamHandle paramshash1 = (OfxParamHandle) "ParamsHash1";
OfxParamHandle paramshash2 = (OfxParamHandle) "ParamsHash2";
OfxParamHandle paramshash3 = (OfxParamHandle) "ParamsHash3";

// These globals hold our copy of the OFX plugin's parameters
// They are updated whenever the OFX plugin feels like it and also before setups are saved
char *dnp_data = NULL;
char *nfp_data = NULL;
int paramshash1_data, paramshash2_data, paramshash3_data = 0;

// This pointer is malloc(d) by the OFX plugin then passed across the API
// to hold its own internal state
void *instancedata = NULL;

// These globals are just for our convenience, because we're often called by the OFX plugin
// in a place where it's awkward to know what the Spark API's current state is
SparkPixelFormat sparkdepth;
int sparkstride = 0;
double sparktime = 0.0;
int sparkw = 0;
int sparkh = 0;
long uniqueid = 0;
char *uniquestring = NULL;
int temporalids[11];

// The Flame shell log will get very cluttered if the OFXWRAP_DEBUG env var is defined :)
#define say if(debug) printf
int debug = 0;

// Each header implements an OFX API "suite" of functions
// More of a "header-only-library" style, they're not so much
// just headers
#include "props.h"
#include "dialogs.h"
#include "ifxs.h"
#include "parms.h"

// Our internal setup save/load functions
#include "setups.h"

// Paths to the plugin binary, a couple options are hard-coded for now
#ifdef __APPLE__
	#define PLUGIN "/Library/OFX/Plugins/NeatVideo4.ofx.bundle/Contents/MacOS/NeatVideo4.ofx"
	#define PLUGIN2 "/usr/discreet/sparks/neat_unpacked/NeatVideo4.ofx.bundle/Contents/MacOS/NeatVideo4.ofx"
#else
	#define PLUGIN "/usr/OFX/Plugins/NeatVideo4.ofx.bundle/Contents/Linux-x86-64/NeatVideo4.ofx"
	#define PLUGIN2 "/usr/discreet/sparks/neat_unpacked/NeatVideo4.ofx.bundle/Contents/Linux-x86-64/NeatVideo4.ofx"
#endif

// Spark UI callback functions forward declares
unsigned long *adjust(int what, SparkInfoStruct si);
unsigned long *prepare(int what, SparkInfoStruct si);

// Spark UI control globals, on page 1
SparkStringStruct SparkString7 = {
	PLUGIN,
	(char *) "%s",
	SPARK_FLAG_NO_INPUT,
	NULL
};
SparkPushStruct SparkPush9 = {
	(char *) "Prepare Noise Profile",
	prepare
};
SparkPushStruct SparkPush10 = {
	(char *) "Adjust Filter Settings",
	adjust
};

// Called by the OFX plugin to retrieve the host's pointers to its OFX API functions
// Each suite's functions are members of a struct defined in each header
const void *fetchSuite(OfxPropertySetHandle host, const char *suite, int version) {
	say("Ofxwrap: fetchSuite() asked for suite %s version %d\n", suite, version);
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

// When things fall apart
void die(const char *format, const char *arg) {
	char *m = (char *) malloc(100);
	sprintf(m, format, arg);
	printf("%s", m);
	sparkError(m);
	sparkProcessAbort();
	free(m);
}

// Tell the OFX plugin to do something, via its mainEntry() function
void action(const char *a, OfxImageEffectHandle e, OfxPropertySetHandle in, OfxPropertySetHandle out) {
	if(plugin == NULL) {
		die("Ofxwrap: cannot do %s, plugin has gone away!\n", a);
	}

	// OS X: crashes inside the OFX plugin binary unless we do this, for unclear reasons.
	// Maybe it's simply using getuid() and getpwuid() to find the home folder to store prefs in,
	// which would return root's home folder because Flame's binary is setuid root, then failing
	// because Flame's effective uid of a normal user doesn't let it write there.
	// On Linux: Qt refuses to create QApplication unless real and effective UID are the same
	// ¯\_(ツ)_/¯
	// An alternative is to change the real uid to the normal user's uid, but that means the Flame
	// process will forever be stuck running as that normal user and unable to switch back to root
	// when it needs to, which seems like a bad idea...
	int realuid = getuid();
	int effectiveuid = geteuid();
	setuid(realuid);

	OfxStatus s = plugin->mainEntry(a, e, in, out);

	// Back to normal please
	seteuid(effectiveuid);

	switch(s) {
		case kOfxStatOK:
			say("Ofxwrap: %s: ok\n", a);
			break;
		case kOfxStatReplyDefault:
			say("Ofxwrap: %s: take default action\n", a);
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
			say("Ofxwrap: %s: returned %d\n", a, s);
	}
}

// Check that a Spark image buffer is ready to use
int bufferReady(int id, SparkMemBufStruct *b) {
	if(!sparkMemGetBuffer(id, b)) {
		say("Ofxwrap: Failed to get buffer %d\n", id);
		return 0;
	}
	if(!(b->BufState & MEMBUF_LOCKED)) {
		say("Ofxwrap: Failed to lock buffer %d\n", id);
		return 0;
	}
	return 1;
}

// Create a new instance of the OFX plugin
void createinstance(void) {
	action(kOfxImageEffectActionDescribeInContext, imageeffecthandle, describeincontextpropsethandle, NULL);
	instancehandle = (OfxImageEffectHandle) "instance";
	action(kOfxActionCreateInstance, instancehandle, NULL, NULL);
	action(kOfxImageEffectActionGetClipPreferences, instancehandle, NULL, NULL);
}

// Flame asks us what extra image buffers we'll want here, we register 11
// extra buffers to use for the plugin's temporal input access
void SparkMemoryTempBuffers(void) {
	if(getenv("OFXWRAP_DEBUG")) debug = 1;

	say("Ofxwrap: in SparkMemoryTempBuffers()...\n");
	for(int i = 0; i < 11; i++) {
		say("Ofxwrap: index %d was %d, ", i, temporalids[i]);
		temporalids[i] = sparkMemRegisterBuffer();
		say("now %d\n", temporalids[i]);
	}
	say("Ofxwrap: ...done with SparkMemoryTempBuffers()\n");
}

// Spark entry function - find the OFX plugin, ask it how it likes to be treated,
// and create an instance of it ready to process images
unsigned int SparkInitialise(SparkInfoStruct si) {
	if(getenv("OFXWRAP_DEBUG")) debug = 1;

	say("\n\n\nOfxwrap: in SparkInitialise(), name is %s\n", si.Name);

	uniquestring = (char *) malloc(100);

	// Where it is?  We could try OFX_PLUGIN_PATH here too I guess
	const char *plugfile = PLUGIN;
	if(access(PLUGIN, F_OK) == -1) {
		plugfile = PLUGIN2;
		strcpy(SparkString7.Value, PLUGIN2);
	}

	// Link the OFX plugin binary into this process
	dlhandle = dlopen(plugfile, RTLD_LAZY);
	if(dlhandle == NULL) {
		die("Ofxwrap: failed to dlopen() OFX plugin %s!\n", plugfile);
		return 0;
	}

	// Find the OfxGetNumberOfPlugins symbol and call it
	int (*OfxGetNumberOfPlugins)(void);
	OfxGetNumberOfPlugins = (int(*)(void)) dlsym(dlhandle, "OfxGetNumberOfPlugins");
	if(OfxGetNumberOfPlugins == NULL) {
		die("Ofxwrap: failed to find plugin's OfxGetNumberOfPlugins symbol!\n", NULL);
		return 0;
	}
	int numplugs = (*OfxGetNumberOfPlugins)();
	say("Ofxwrap: found %d plugins\n", numplugs);

	// Find the OfxGetPlugin symbol and call it for the first plugin
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
	say("Ofxwrap: plugin id is %s\n", plugin->pluginIdentifier);

	// The host struct h describes the host to the OFX plugin
	h.host = hostpropsethandle;
	h.fetchSuite = &fetchSuite;
	plugin->setHost(&h);

	// Now we can call the OFX plugin's own init functions via OFX actions
	// This causes the OFX plugin to call a whole slew of our API functions
	// to describe itself
	action(kOfxActionLoad, NULL, NULL, NULL);
	action(kOfxActionDescribe, imageeffecthandle, NULL, NULL);
	createinstance();

	return(SPARK_MODULE);
}

// These bit-depth conversion loops are screaming out to be threaded via sparkMpFork()
void rgb16fp_to_rgba32fp(char *in, int stride, int inc, float *out) {
	for(int y = 0; y < sparkh; y++) {
		for(int x = 0; x < sparkw; x++) {
			out[y * sparkw * 4 + x * 4 + 0] = *(half *)(in + stride * y + inc * x + 0);
			out[y * sparkw * 4 + x * 4 + 1] = *(half *)(in + stride * y + inc * x + 2);
			out[y * sparkw * 4 + x * 4 + 2] = *(half *)(in + stride * y + inc * x + 4);
			out[y * sparkw * 4 + x * 4 + 3] = 1.0;
		}
	}
}
void rgb16int_to_rgba32fp(char *in, int stride, int inc, float *out) {
	for(int y = 0; y < sparkh; y++) {
		for(int x = 0; x < sparkw; x++) {
			out[y * sparkw * 4 + x * 4 + 0] = *(unsigned short *)(in + stride * y + inc * x + 0) / 65535.0;
			out[y * sparkw * 4 + x * 4 + 1] = *(unsigned short *)(in + stride * y + inc * x + 2) / 65535.0;
			out[y * sparkw * 4 + x * 4 + 2] = *(unsigned short *)(in + stride * y + inc * x + 4) / 65535.0;
			out[y * sparkw * 4 + x * 4 + 3] = 1.0;
		}
	}
}
void rgb8int_to_rgba32fp(char *in, int stride, int inc, float *out) {
	for(int y = 0; y < sparkh; y++) {
		for(int x = 0; x < sparkw; x++) {
			out[y * sparkw * 4 + x * 4 + 0] = *(unsigned char *)(in + stride * y + inc * x + 0) / 255.0;
			out[y * sparkw * 4 + x * 4 + 1] = *(unsigned char *)(in + stride * y + inc * x + 1) / 255.0;
			out[y * sparkw * 4 + x * 4 + 2] = *(unsigned char *)(in + stride * y + inc * x + 2) / 255.0;
			out[y * sparkw * 4 + x * 4 + 3] = 1.0;
		}
	}
}

// The OFX plugin can return float pixels outside of the input range which is a problem if the Spark is
// running at an integer bit-depth
float clamp(float d, float min, float max) {
  const float t = d < min ? min : d;
  return t > max ? max : t;
}

// Spark entry point for each frame to be rendered
unsigned long *SparkProcess(SparkInfoStruct si) {
	say("Ofxwrap: in SparkProcess(), name is %s\n", si.Name);

	if(dnp_data == NULL || nfp_data == NULL) {
		say("Ofxwrap: in SparkProcess(), DNP or NFP is null\n");
	} else {
		say("Ofxwrap: in SparkProcess(), DNP %.10s, NFP %.10s, hashes %d %d %d\n", dnp_data, nfp_data, paramshash1_data, paramshash2_data, paramshash3_data);
	}

	// Check Spark image buffers are ready for use
	SparkMemBufStruct result, front, temporalbuffers[11];
	if(!bufferReady(1, &result)) return(NULL);
	if(!bufferReady(2, &front)) return(NULL);
	for(int i = 0; i < 11; i++) {
		if(!bufferReady(temporalids[i], &temporalbuffers[i])) return(NULL);
	}

	// If the resolution changes, we're gonna have a bad time here...
	sparkw = front.BufWidth;
	sparkh = front.BufHeight;
	sparkdepth = (SparkPixelFormat) front.BufDepth;
	sparktime = si.FrameNo;

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


	// Make current frame input buffer to feed to the OFX plugin, big enough for an RGBA 32-bit float image
	if(currentframeimagehandle == NULL) {
		currentframeimagehandle = (OfxPropertySetHandle) malloc(sparkw * sparkh * 4 * 4);
	}
	// Convert whatever the Spark input pixels are to RGBA 32-bit float for the OFX plugin
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
	say("Ofxwrap: in SparkProcess(), currentframe is %p\n", currentframeimagehandle);
	say("Ofxwrap: in SparkProcess(), front buffer is %p\n", front.Buffer);

	// Same buffer creation and conversion for each temporal input image
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

	// Make a buffer ready for the OFX plugin's output
	if(outputimagehandle == NULL) {
		outputimagehandle = (OfxPropertySetHandle) malloc(sparkw * sparkh * 4 * 4);
	}
	say("Ofxwrap: in SparkProcess(), outputframe is %p\n", outputimagehandle);
	say("Ofxwrap: in SparkProcess(), result buffer is %p\n", result.Buffer);

	// Tell the OFX plugin to process this frame
	action(kOfxImageEffectActionBeginSequenceRender, instancehandle, beginseqpropsethandle, NULL);
	action(kOfxImageEffectActionRender, instancehandle, renderpropsethandle, NULL);
	action(kOfxImageEffectActionEndSequenceRender, instancehandle, beginseqpropsethandle, NULL);

	// Convert OFX plugin's RGBA 32-bit float output to our Spark's current pixel depth
	// Again, should be doable via sparkMpFork()
	char *rb = (char *) result.Buffer;
	float *oih = (float *) outputimagehandle;
	switch(sparkdepth) {
		case SPARKBUF_RGB_48_3x16_FP:
			for(int y = 0; y < sparkh; y++) {
				for(int x = 0; x < sparkw; x++) {
					*(half *)(rb + result.Stride * y + result.Inc * x + 0) = oih[y * sparkw * 4 + x * 4 + 0];
					*(half *)(rb + result.Stride * y + result.Inc * x + 2) = oih[y * sparkw * 4 + x * 4 + 1];
					*(half *)(rb + result.Stride * y + result.Inc * x + 4) = oih[y * sparkw * 4 + x * 4 + 2];
				}
			}
		break;
		case SPARKBUF_RGB_48_3x10:
		case SPARKBUF_RGB_48_3x12:
			for(int y = 0; y < sparkh; y++) {
				for(int x = 0; x < sparkw; x++) {
					*(unsigned short *)(rb + result.Stride * y + result.Inc * x + 0) = clamp(oih[y * sparkw * 4 + x * 4 + 0], 0, 1) * 65535.0;
					*(unsigned short *)(rb + result.Stride * y + result.Inc * x + 2) = clamp(oih[y * sparkw * 4 + x * 4 + 1], 0, 1) * 65535.0;
					*(unsigned short *)(rb + result.Stride * y + result.Inc * x + 4) = clamp(oih[y * sparkw * 4 + x * 4 + 2], 0, 1) * 65535.0;
				}
			}
		break;
		case SPARKBUF_RGB_24_3x8:
			for(int y = 0; y < sparkh; y++) {
				for(int x = 0; x < sparkw; x++) {
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

// Spark deletion entry point.  Note this is often called much later than you'd imagine, like
// not when you delete the Batch node but some minutes later when leaving Batch or switching setups
void SparkUnInitialise(SparkInfoStruct si) {
	say("Ofxwrap: in SparkUnInitialise(), name is %s\n", si.Name);

	// Gonna have to just leak instances here
	// Calling this destroy action causes the plugin to try to access things
	// from other instances which may no longer exist, probably the host struct stuff
	// if(instancehandle != NULL) action(kOfxActionDestroyInstance, instancehandle, NULL, NULL);

	// Free all our globals
	instancehandle = NULL;
	instancedata = NULL;
	free(uniquestring); uniquestring = NULL;
	free(currentframeimagehandle); currentframeimagehandle = NULL;
	free(outputimagehandle); outputimagehandle = NULL;
	free(dnp_data);	dnp_data = NULL;
	free(nfp_data);	nfp_data = NULL;
	for(int i = 0; i < 11; i++) {
		free(temporalframeimagehandles[i]); temporalframeimagehandles[i] = NULL;
	}

	// We can't do this because other instances of the Spark may be alive still
	// action(kOfxActionUnload, NULL, NULL, NULL);

	// It's safe to try to unlink the plugin binary because the handles are
	// reference counted globally for the process, and won't be closed completely
	// if any are left open
	int r = dlclose(dlhandle);
	if(r == 0) {
		say("Ofxwrap: dlclose() ok\n");
		dlhandle = NULL;
	} else {
		say("Ofxwrap: dlclose() failed! Reason: %s\n", dlerror());
	}
}

// Spark entry point called when a Spark setup is saved or loaded
// This is also called when Flame needs to save/load/copy the Spark for reasons of its own,
// like when copying timeline FX or autosaving Batch, so it's essential we can save and
// restore our complete state here.  We do it by creating an extra setup file next to the
// regular Spark setup, the one which Flame handles for us
void SparkSetupIOEvent(SparkModuleEvent e, char *path, char *file) {
	say("Ofxwrap: in SparkIOEvent(), event is %d, path is %s, file is %s\n", (int)e, path, file);
	if(e == SPARK_EVENT_SAVESETUP) {
		if(instancehandle == NULL) return;
		action(kOfxActionSyncPrivateData, instancehandle, NULL, NULL);
		setups_save(path, file);
	} else if(e == SPARK_EVENT_LOADSETUP) {
		setups_load(path, file);

		/* This worked for the filter page settings but not the noise profile, the change was ignored
		action(kOfxActionBeginInstanceChanged, instancehandle, begininstancechangepropsethandle, NULL);
		action(kOfxActionInstanceChanged, instancehandle, dnpchangepropsethandle, NULL);
		action(kOfxActionInstanceChanged, instancehandle, nfpchangepropsethandle, NULL);
		action(kOfxActionInstanceChanged, instancehandle, paramshash1changepropsethandle, NULL);
		action(kOfxActionInstanceChanged, instancehandle, paramshash2changepropsethandle, NULL);
		action(kOfxActionInstanceChanged, instancehandle, paramshash3changepropsethandle, NULL);
		action(kOfxActionEndInstanceChanged, instancehandle, endinstancechangepropsethandle, NULL); */

		// So just create a new instance, which will read the current state
		if(plugin != NULL) {
			createinstance();
		}
	}
}

// Button-click callback for the first button
unsigned long *prepare(int what, SparkInfoStruct si) {
	// Tell the OFX plugin that this button's state has changed
	action(kOfxActionBeginInstanceChanged, instancehandle, begininstancechangepropsethandle, NULL);
	action(kOfxActionInstanceChanged, instancehandle, preparebuttonchangepropsethandle, NULL);
	action(kOfxActionEndInstanceChanged, instancehandle, endinstancechangepropsethandle, NULL);
	sparkReprocess();
	return NULL;
}
// Button-click callback for the second button
unsigned long *adjust(int what, SparkInfoStruct si) {
	// Tell the OFX plugin that this button's state has changed
	action(kOfxActionBeginInstanceChanged, instancehandle, begininstancechangepropsethandle, NULL);
	action(kOfxActionInstanceChanged, instancehandle, adjustbuttonchangepropsethandle, NULL);
	action(kOfxActionEndInstanceChanged, instancehandle, endinstancechangepropsethandle, NULL);
	sparkReprocess();
	return NULL;
}

// Called by Flame to find out what bit-depths we support... all of them :)
int SparkIsInputFormatSupported(SparkPixelFormat fmt) {
	switch(fmt) {
		case SPARKBUF_RGB_24_3x8:
		case SPARKBUF_RGB_48_3x10:
		case SPARKBUF_RGB_48_3x12:
		case SPARKBUF_RGB_48_3x16_FP:
			return 1;
		break;
		default:
			say("Ofxwrap: SparkIsInputFormatSupported(), unhandled pixel depth %d, failing!\n", fmt);
			return 0;
	}
}

// Called by Flame for assorted UI events
void SparkEvent(SparkModuleEvent e) {
	say("Ofxwrap: in SparkEvent(), event is %d, last setup name is %s\n", (int)e, sparkGetLastSetupName());
	if(e == SPARK_EVENT_RESULT) {
		// For some reason the result view is not always updated when scrubbing in Front view then
		// hitting F4.  Shame because this makes F1/F4 Front/Result flipping slow
		sparkReprocess();
	}
}

// Called by Flame to find out how many input clips we require
int SparkClips(void) {
	return 1;
}
