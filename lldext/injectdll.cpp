#include <string>

#include <Windows.h>
#include <DbgEng.h>

#include <wil/resource.h>
#include <wil/result.h>
#include <wil/com.h>

namespace lldext
{
class InjectionControl : public DebugBaseEventCallbacks
{
private:

protected:
    wil::com_ptr_t<IDebugClient> _debug_client;
    wil::com_ptr_t<IDebugSystemObjects> _debug_system_objects;
    wil::unique_virtualalloc_ptr<BYTE> _remotebuffer;

    DWORD _remoteThreadId;

    int _instance_count{};

public:
    InjectionControl(IDebugClient* debug_client) : _remoteThreadId{}, _debug_client{ debug_client } {
        THROW_IF_FAILED(_debug_client->QueryInterface(__uuidof(IDebugSystemObjects),
            reinterpret_cast<PVOID*>(_debug_system_objects.put())));
    }

    ~InjectionControl(void) {}

    HRESULT __stdcall Inject(const std::string& dll_name) {
        wil::com_ptr_t<IDebugControl> debug_control{};
        RETURN_IF_FAILED(_debug_client->QueryInterface(__uuidof(IDebugControl), reinterpret_cast<PVOID*>(&debug_control)));
        wil::com_ptr_t<IDebugSymbols> debug_symbols{};
        RETURN_IF_FAILED(_debug_client->QueryInterface(__uuidof(IDebugSymbols), reinterpret_cast<PVOID*>(&debug_symbols)));

        // find the LoadLibrary function (on Win7 we need to use kernel32, on Win8+ kernelbase)
        ULONG64 offset;
        if (FAILED(debug_symbols->GetOffsetByName("kernelbase!LoadLibraryA", &offset))) {
            RETURN_IF_FAILED(debug_symbols->GetOffsetByName("kernel32!LoadLibraryA", &offset));
        }

        ULONG64 h;
        RETURN_IF_FAILED(_debug_system_objects->GetCurrentProcessHandle(&h));
        auto process_handle{ reinterpret_cast<HANDLE>(h) };

        _remotebuffer.reset(reinterpret_cast<PBYTE>(::VirtualAllocEx(process_handle, nullptr, dll_name.size() + 1, MEM_COMMIT, PAGE_EXECUTE_READWRITE)));
        RETURN_LAST_ERROR_IF_NULL(_remotebuffer.get());

        SIZE_T n;
        RETURN_IF_WIN32_BOOL_FALSE(WriteProcessMemory(process_handle, _remotebuffer.get(), dll_name.c_str(), dll_name.size() + 1, &n));
        if (n != dll_name.size() + 1) {
            return E_FAIL;
        }

        wil::unique_handle thread_handle{ ::CreateRemoteThread(process_handle, nullptr, 0, (LPTHREAD_START_ROUTINE)offset, _remotebuffer.get(), 0, &_remoteThreadId) };

        RETURN_IF_FAILED(_debug_client->SetEventCallbacks(this));
        debug_control->Output(DEBUG_OUTPUT_NORMAL, "The DLL will be loaded when you continue the process execution.\n");

        return S_OK;
    }

    virtual HRESULT __stdcall QueryInterface(REFIID interfaceId, PVOID* instance) override {
        if (interfaceId == __uuidof(IUnknown) || interfaceId == __uuidof(IDebugEventCallbacks)) {
            *instance = this;
            AddRef();
            return S_OK;
        } else {
            *instance = NULL;
            return E_NOINTERFACE;
        }
    }

    virtual ULONG __stdcall AddRef(void) override {
        _instance_count += 1;
        return _instance_count;
    }

    virtual ULONG __stdcall Release(void) override {
        int cnt = --_instance_count;
        if (cnt == 0) {
            delete this;
        }
        return cnt;
    }

    virtual HRESULT __stdcall GetInterestMask(PULONG Mask) override {

        *Mask = DEBUG_EVENT_EXIT_THREAD;
        return S_OK;
    }

    virtual HRESULT __stdcall ExitThread(ULONG) {
        DWORD threadId;
        RETURN_IF_FAILED(_debug_system_objects->GetCurrentThreadSystemId(&threadId));
        if (threadId == _remoteThreadId) {
            // calls Release and removes the component
            _debug_client->SetEventCallbacks(nullptr);
            return DEBUG_STATUS_BREAK;
        }
        return DEBUG_STATUS_NO_CHANGE;
    }
};
}

extern "C" HRESULT CALLBACK injectdll(IDebugClient * debugclient, PCSTR args) {
    wil::com_ptr_t<IDebugControl> pDebugControl{};
    RETURN_IF_FAILED(debugclient->QueryInterface(__uuidof(IDebugControl), pDebugControl.put_void()));

    std::string path{ args };
    if (path.empty()) {
        pDebugControl->Output(DEBUG_OUTPUT_ERROR, "DLL path is required.\n");
        return E_INVALIDARG;
    }

    wil::com_ptr_t<IDebugClient> ndebugclient;
    RETURN_IF_FAILED(debugclient->CreateClient(ndebugclient.put()));
    try {
        wil::com_ptr_t<lldext::InjectionControl> injection_engine{ new lldext::InjectionControl(ndebugclient.get()) };
        return injection_engine->Inject(args);
    } catch (...) {
        RETURN_CAUGHT_EXCEPTION();
    }
}
