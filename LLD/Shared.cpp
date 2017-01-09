#include "DbgExts.h"

void CheckHResult(HRESULT const result) {
    if (result != S_OK) {
        throw Exception(result);
    }
}

HRESULT CALLBACK help(IDebugClient* pDebugClient, PCSTR args) {
    IDebugControl* pDebugControl;
    UNREFERENCED_PARAMETER(args);
    if (SUCCEEDED(pDebugClient->QueryInterface(__uuidof(IDebugControl), (void **)&pDebugControl))) {
        pDebugControl->Output(DEBUG_OUTPUT_NORMAL, "==============================================================\n");
        pDebugControl->Output(DEBUG_OUTPUT_NORMAL, "  LLD v1.3 - Copyright 2017 Sebastian Solnica\n");
        pDebugControl->Output(DEBUG_OUTPUT_NORMAL, "==============================================================\n\n");
        pDebugControl->Output(DEBUG_OUTPUT_NORMAL, "  !help\n");
        pDebugControl->Output(DEBUG_OUTPUT_NORMAL, "   - shows this help view\n\n");
        pDebugControl->Output(DEBUG_OUTPUT_NORMAL, "  !injectdll <path>\n");
        pDebugControl->Output(DEBUG_OUTPUT_NORMAL, "   - injects DLL into the debugee\n\n");
        pDebugControl->Output(DEBUG_OUTPUT_NORMAL, "  !ufgraph <function-address-or-name> [<code-addr>]\n");
        pDebugControl->Output(DEBUG_OUTPUT_NORMAL, "   - runs ufgraph on a given function (please check the documentation for requirements)\n");
        pDebugControl->Output(DEBUG_OUTPUT_NORMAL, "     ufgraph.py can be downloaded from github: https://github.com/bfosterjr/ufgraph\n\n");
        pDebugControl->Output(DEBUG_OUTPUT_NORMAL, "==============================================================\n\n");

        pDebugControl->Release();
    }
    return S_OK;
}
