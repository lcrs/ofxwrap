// Implements the OFX API's Dialog suite functions, which we don't react to
// but need to exist
OfxStatus dialogs_RequestDialog(void *user_data) {
	say("Ofxwrap: in dialogs_RequestDialog(), user data is %p\n", user_data);
	return kOfxStatOK;
}

OfxStatus dialogs_NotifyRedrawPending(void) {
	say("Ofxwrap: in dialogs_NotifyRedrawPending()\n");
	return kOfxStatOK;
}

OfxDialogSuiteV1 dialogs;

void dialogs_init(void) {
	say("Ofxwrap: in dialogs_init()\n");
	dialogs.RequestDialog = dialogs_RequestDialog;
	dialogs.NotifyRedrawPending = dialogs_NotifyRedrawPending;
}
