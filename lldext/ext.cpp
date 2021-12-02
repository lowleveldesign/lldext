
#include <DbgEng.h>

#define EXT_MAJOR_VER	1
#define EXT_MINOR_VER	3

extern "C" HRESULT CALLBACK
DebugExtensionInitialize(PULONG version, PULONG flags) {
	*version = DEBUG_EXTENSION_VERSION(EXT_MAJOR_VER, EXT_MINOR_VER);
	*flags = 0;  // Reserved for future use.
	return S_OK;
}

extern "C" void CALLBACK
DebugExtensionNotify(ULONG notify, ULONG64 argument) {
	UNREFERENCED_PARAMETER(argument);

	switch (notify) {
		// A debugging session is active. The session may not necessarily be suspended.
	case DEBUG_NOTIFY_SESSION_ACTIVE:
		break;
		// No debugging session is active.
	case DEBUG_NOTIFY_SESSION_INACTIVE:
		break;
		// The debugging session has suspended and is now accessible.
	case DEBUG_NOTIFY_SESSION_ACCESSIBLE:
		break;
		// The debugging session has started running and is now inaccessible.
	case DEBUG_NOTIFY_SESSION_INACCESSIBLE:
		break;
	}
	return;
}

extern "C" void CALLBACK
DebugExtensionUninitialize(void) {
	return;
}

HRESULT CALLBACK help(IDebugClient* dbgclient, PCSTR args) {
	UNREFERENCED_PARAMETER(args);

	IDebugControl* dbgctrl;
	if (SUCCEEDED(dbgclient->QueryInterface(__uuidof(IDebugControl), (void**)&dbgctrl))) {
		dbgctrl->Output(DEBUG_OUTPUT_NORMAL, "==============================================================\n");
		dbgctrl->Output(DEBUG_OUTPUT_NORMAL, "  LLD v1.3 - Copyright 2017 Sebastian Solnica\n");
		dbgctrl->Output(DEBUG_OUTPUT_NORMAL, "==============================================================\n\n");
		dbgctrl->Output(DEBUG_OUTPUT_NORMAL, "  !help\n");
		dbgctrl->Output(DEBUG_OUTPUT_NORMAL, "   - shows this help view\n\n");
		dbgctrl->Output(DEBUG_OUTPUT_NORMAL, "  !injectdll <path>\n");
		dbgctrl->Output(DEBUG_OUTPUT_NORMAL, "   - injects DLL into the debugee\n\n");
		dbgctrl->Output(DEBUG_OUTPUT_NORMAL, "==============================================================\n\n");

		dbgctrl->Release();
	}
	return S_OK;
}

