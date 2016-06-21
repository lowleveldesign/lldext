// dbgexts.h
#pragma once

#include <windows.h>
#include <dbgeng.h>
#include <strsafe.h>

#define EXT_MAJOR_VER	1
#define EXT_MINOR_VER	0


struct Exception
{
    HRESULT Result;

    explicit Exception(HRESULT const result) noexcept : Result(result) {}
};

void CheckHResult(HRESULT const result);

class InjectionControl : public IDebugEventCallbacks
{
private:

protected:
    IDebugClient *m_pDebugClient;
    IDebugControl *m_pDebugControl;
    IDebugSystemObjects *m_pDebugSystemObjects;
    IDebugSymbols *m_pDebugSymbols;
    IDebugAdvanced *m_pDebugAdvanced;
    IDebugRegisters *m_pDebugRegisters;
    CONTEXT m_threadContext;

    virtual void PrepareInjectionContext(CONTEXT *pTempContext, PBYTE injectionBuffer, 
        size_t dllNameLength, ULONG64 offset) = 0;

    virtual bool IsBreakpointOffsetHit(void) = 0;

    virtual PBYTE GetPayload(void) = 0;

    virtual size_t GetPayloadSize(void) = 0;

    void SuspendAllThreadsButCurrent(void);

    void ResumeAllThreads(void);

public:
    InjectionControl(IDebugClient *pOriginalDebugClient);

    ~InjectionControl(void);

    void Inject(PCSTR dllName);

    // IDebugEventsCallbacks
    STDMETHOD(QueryInterface)(REFIID InterfaceId, PVOID* Interface);

    STDMETHOD_(ULONG, AddRef)();

    STDMETHOD_(ULONG, Release)();

    STDMETHOD(GetInterestMask)(PULONG Mask);

    STDMETHOD(Breakpoint)(PDEBUG_BREAKPOINT Bp);

    STDMETHOD(Exception)(PEXCEPTION_RECORD64 Exception, ULONG FirstChance);

    STDMETHOD(CreateThread)(ULONG64 Handle, ULONG64 DataOffset, ULONG64 StartOffset);

    STDMETHOD(ExitThread)(ULONG ExitCode);

    STDMETHOD(CreateProcess)(ULONG64 ImageFileHandle, ULONG64 Handle,
        ULONG64 BaseOffset, ULONG ModuleSize, PCSTR ModuleName, PCSTR ImageName,
        ULONG CheckSum, ULONG TimeDateStamp, ULONG64 InitialThreadHandle, ULONG64 ThreadDataOffset,
        ULONG64 StartOffset);

    STDMETHOD(ExitProcess)(ULONG ExitCode);

    STDMETHOD(LoadModule)(ULONG64 ImageFileHandle, ULONG64 BaseOffset, ULONG ModuleSize,
        PCSTR ModuleName, PCSTR ImageName, ULONG CheckSum, ULONG TimeDateStamp);

    STDMETHOD(UnloadModule)(__in_opt PCSTR ImageBaseName, ULONG64 BaseOffset);

    STDMETHOD(SystemError)(ULONG Error, ULONG Level);

    STDMETHOD(SessionStatus)(ULONG Status);

    STDMETHOD(ChangeDebuggeeState)(ULONG Flags, ULONG64 Argument);

    STDMETHOD(ChangeEngineState)(ULONG Flags, ULONG64 Argument);

    STDMETHOD(ChangeSymbolState)(ULONG Flags, ULONG64 Argument);
};

#if _WIN64
#define PAYLOAD_SIZE 3
#else
#define PAYLOAD_SIZE 4
#endif

class InjectionControlImpl final : public InjectionControl
{
private:
    static BYTE m_payload[PAYLOAD_SIZE];
    DWORD64 m_breakOffset;

protected:
    void PrepareInjectionContext(CONTEXT *pTempContext, PBYTE injectionBuffer, 
        size_t dllNameLength, ULONG64 offset);

    bool IsBreakpointOffsetHit(void);

    PBYTE GetPayload(void);

    size_t GetPayloadSize(void);

public:

    InjectionControlImpl(IDebugClient *pDebugClient) : InjectionControl(pDebugClient) {}
};
