#include "openfx/include/ofxDialog.h"

OfxStatus dialogs_RequestDialog(void *user_data) {
	printf("Ofxwrap: in dialogs_RequestDialog(), user data is %p\n", user_data);
	return kOfxStatOK;
}

OfxStatus dialogs_NotifyRedrawPending(void) {
	printf("Ofxwrap: in dialogs_NotifyRedrawPending()\n");
	return kOfxStatOK;
}

OfxDialogSuiteV1 dialogs;

void dialogs_init(void) {
	printf("Ofxwrap: in dialogs_init()\n");
	dialogs.RequestDialog = dialogs_RequestDialog;
	dialogs.NotifyRedrawPending = dialogs_NotifyRedrawPending;
}
