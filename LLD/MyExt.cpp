// MyExt.cpp

#include "dbgext.h"

HRESULT CALLBACK 
helloworld(IDebugClient* pDebugClient, PCSTR args)
{
	UNREFERENCED_PARAMETER(args);

	IDebugControl* pDebugControl;
	if (SUCCEEDED(pDebugClient->QueryInterface(__uuidof(IDebugControl), (void **)&pDebugControl)))
	{
		pDebugControl->Output(DEBUG_OUTPUT_NORMAL, "Hello World!\n");
		pDebugControl->Release();
	}
	return S_OK;
}

HRESULT CALLBACK 
helloworldold(IDebugClient* pDebugClient, PCSTR args)
{
    UNREFERENCED_PARAMETER(pDebugClient);
    UNREFERENCED_PARAMETER(args);

	dprintf("Hello World!\n");

	return S_OK;
}

HRESULT CALLBACK 
expression(IDebugClient* pDebugClient, PCSTR args)
{
	IDebugControl* pDebugControl;
	if (SUCCEEDED(pDebugClient->QueryInterface(__uuidof(IDebugControl), (void **)&pDebugControl)))
	{
		DEBUG_VALUE Value = {0};
		pDebugControl->Evaluate(args, DEBUG_VALUE_INT64, &Value, NULL);

		pDebugControl->Output(DEBUG_OUTPUT_NORMAL, "%s = %p\n", args, Value.I64);
		pDebugControl->Release();
	}
	return S_OK;
}

HRESULT CALLBACK 
expressionold(IDebugClient* pDebugClient, PCSTR args)
{
    UNREFERENCED_PARAMETER(pDebugClient);
    ULONG64 ulAddr = GetExpression(args);

	dprintf("%s = %p\n", args, ulAddr);

	return S_OK;
}
