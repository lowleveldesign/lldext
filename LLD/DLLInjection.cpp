
#include "DbgExts.h"

HRESULT CALLBACK injectdll(IDebugClient* pDebugClient, PCSTR args)
{
    IDebugControl* pDebugControl;
    if (SUCCEEDED(pDebugClient->QueryInterface(__uuidof(IDebugControl), (void **)&pDebugControl))) {
        if (strlen(args) == 0) {
            pDebugControl->Output(DEBUG_OUTPUT_NORMAL, "DLL path is required.");
            pDebugControl->Release();
            return S_OK;
        }
        InjectionControl *pInjectionEngine = new InjectionControl(pDebugClient);
        pInjectionEngine->Inject(args);

        pDebugControl->Release();
    }
    return S_OK;
}


InjectionControl::InjectionControl(IDebugClient * pOriginalDebugClient)
{
    CheckHResult(pOriginalDebugClient->CreateClient(&m_pDebugClient));
    CheckHResult(m_pDebugClient->QueryInterface(__uuidof(IDebugControl), (void **)&m_pDebugControl));
    CheckHResult(m_pDebugClient->QueryInterface(__uuidof(IDebugSystemObjects), (void **)&m_pDebugSystemObjects));
    CheckHResult(m_pDebugClient->QueryInterface(__uuidof(IDebugSymbols), (void **)&m_pDebugSymbols));
    CheckHResult(m_pDebugClient->QueryInterface(__uuidof(IDebugAdvanced), (void **)&m_pDebugAdvanced));
    CheckHResult(m_pDebugClient->QueryInterface(__uuidof(IDebugRegisters), (void **)&m_pDebugRegisters));
}

InjectionControl::~InjectionControl(void)
{
    m_pDebugClient->Release();
    m_pDebugControl->Release();
    m_pDebugSystemObjects->Release();
    m_pDebugSymbols->Release();
    m_pDebugAdvanced->Release();
    m_pDebugRegisters->Release();
}

STDMETHODIMP InjectionControl::QueryInterface(REFIID interfaceId, PVOID* instance)
{
    if (interfaceId == __uuidof(IUnknown) || interfaceId == __uuidof(IDebugEventCallbacks)) {
        *instance = this;
        // No need to refcount as this class is contained.
        return S_OK;
    } else {
        *instance = NULL;
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG) InjectionControl::AddRef() { return S_OK; }

STDMETHODIMP_(ULONG) InjectionControl::Release() { return S_OK; }

void InjectionControl::SuspendAllThreadsButCurrent(void)
{
    m_pDebugControl->Execute(DEBUG_OUTPUT_NORMAL, "~*n", DEBUG_EXECUTE_NOT_LOGGED);
}

void InjectionControl::ResumeAllThreads(void)
{
    m_pDebugControl->Execute(DEBUG_OUTPUT_NORMAL, "~*m", DEBUG_EXECUTE_NOT_LOGGED);
}

void InjectionControl::Inject(PCSTR dllName)
{
    ULONG64 hProcess;

    // find the LoadLibrary function (on Win7 we need to use kernel32, on Win8+ kernelbase)
    ULONG64 offset;
    if (FAILED(m_pDebugSymbols->GetOffsetByName("kernelbase!LoadLibraryA", &offset))) {
        CheckHResult(m_pDebugSymbols->GetOffsetByName("kernel32!LoadLibraryA", &offset));
    }

    CheckHResult(m_pDebugSystemObjects->GetCurrentProcessHandle(&hProcess));

    size_t dllNameLength = strlen(dllName) + 1;
    SIZE_T n;
    // allocate injection buffer
    PBYTE injectionBuffer = (PBYTE)VirtualAllocEx((HANDLE)hProcess, nullptr, dllNameLength,
        MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (!injectionBuffer) {
        throw ::Exception(HRESULT_FROM_WIN32(GetLastError()));
    }
    if (!WriteProcessMemory((HANDLE)hProcess, injectionBuffer, dllName, dllNameLength, &n)) {
        throw ::Exception(HRESULT_FROM_WIN32(GetLastError()));
    }

    HANDLE hThread;
    hThread = CreateRemoteThread((HANDLE)hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)offset, 
        injectionBuffer, 0, &m_remoteThreadId);
    CloseHandle(hThread);

    CheckHResult(m_pDebugClient->SetEventCallbacks(this));

    // run the only thread - should break after the injection
    CheckHResult(m_pDebugControl->Execute(DEBUG_OUTPUT_NORMAL, "g", DEBUG_EXECUTE_NOT_LOGGED));
}

STDMETHODIMP InjectionControl::GetInterestMask(PULONG Mask)
{
    *Mask = DEBUG_EVENT_EXIT_THREAD;
    return S_OK;
}

STDMETHODIMP InjectionControl::Breakpoint(PDEBUG_BREAKPOINT Bp)
{
    return DEBUG_STATUS_GO;
}

STDMETHODIMP InjectionControl::Exception(PEXCEPTION_RECORD64 Exception, ULONG FirstChance)
{
    return DEBUG_STATUS_GO;
}

STDMETHODIMP InjectionControl::CreateThread(ULONG64 Handle, ULONG64 DataOffset,
    ULONG64 StartOffset)
{
    return DEBUG_STATUS_GO;
}

STDMETHODIMP InjectionControl::ExitThread(ULONG ExitCode)
{
    DWORD threadId;
    CheckHResult(m_pDebugSystemObjects->GetCurrentThreadSystemId(&threadId));
    if (threadId == m_remoteThreadId) {
        m_pDebugClient->SetEventCallbacks(nullptr);
        delete this;

        return DEBUG_STATUS_BREAK;
    }
    return DEBUG_STATUS_GO;
}

STDMETHODIMP InjectionControl::CreateProcess(ULONG64 ImageFileHandle, ULONG64 Handle, ULONG64 BaseOffset,
    ULONG ModuleSize, PCSTR ModuleName, PCSTR ImageName, ULONG CheckSum, ULONG TimeDateStamp,
    ULONG64 InitialThreadHandle, ULONG64 ThreadDataOffset, ULONG64 StartOffset)
{
    return DEBUG_STATUS_GO;
}

STDMETHODIMP InjectionControl::ExitProcess(ULONG ExitCode)
{
    return DEBUG_STATUS_GO;
}

// Any of these values may be zero.
STDMETHODIMP InjectionControl::LoadModule(ULONG64 ImageFileHandle, ULONG64 BaseOffset, ULONG ModuleSize,
    PCSTR ModuleName, PCSTR ImageName, ULONG CheckSum, ULONG TimeDateStamp)
{
    return DEBUG_STATUS_GO;
}

STDMETHODIMP InjectionControl::UnloadModule(__in_opt PCSTR ImageBaseName, ULONG64 BaseOffset)
{
    return DEBUG_STATUS_GO;
}

STDMETHODIMP InjectionControl::SystemError(ULONG Error, ULONG Level)
{
    return DEBUG_STATUS_GO;
}

STDMETHODIMP InjectionControl::SessionStatus(ULONG Status)
{
    return DEBUG_STATUS_NO_CHANGE;
}

STDMETHODIMP InjectionControl::ChangeDebuggeeState(ULONG Flags, ULONG64 Argument)
{
    return DEBUG_STATUS_NO_CHANGE;
}

STDMETHODIMP InjectionControl::ChangeEngineState(ULONG Flags, ULONG64 Argument)
{
    return DEBUG_STATUS_NO_CHANGE;
}

STDMETHODIMP InjectionControl::ChangeSymbolState(ULONG Flags, ULONG64 Argument)
{
    return DEBUG_STATUS_NO_CHANGE;
}
