#include <Windows.h>
#include <DbgEng.h>
#include <string>
#include <wil/resource.h>
#include <wil/result.h>
#include <wil/com.h>

namespace lldext
{
	class InjectionControl : public DebugBaseEventCallbacks
	{
	private:

	protected:
		wil::com_ptr_t<IDebugClient> _pDebugClient;
		wil::com_ptr_t<IDebugSystemObjects> _pDebugSystemObjects;
		wil::unique_virtualalloc_ptr<BYTE> _remotebuffer;

		DWORD m_remoteThreadId;

		int _instance_count{};

	public:
		InjectionControl(IDebugClient* pDebugClient) : m_remoteThreadId{}, _pDebugClient{ pDebugClient } {
			THROW_IF_FAILED(_pDebugClient->QueryInterface(__uuidof(IDebugSystemObjects),
				reinterpret_cast<PVOID*>(_pDebugSystemObjects.put())));
		}

		~InjectionControl(void) {}

		HRESULT __stdcall Inject(const std::string& dll_name) {
			wil::com_ptr_t<IDebugControl> debugcontrol{};
			RETURN_IF_FAILED(_pDebugClient->QueryInterface(__uuidof(IDebugControl), reinterpret_cast<PVOID*>(&debugcontrol)));
			wil::com_ptr_t<IDebugSymbols> debugsymbols{};
			RETURN_IF_FAILED(_pDebugClient->QueryInterface(__uuidof(IDebugSymbols), reinterpret_cast<PVOID*>(&debugsymbols)));

			// find the LoadLibrary function (on Win7 we need to use kernel32, on Win8+ kernelbase)
			ULONG64 offset;
			if (FAILED(debugsymbols->GetOffsetByName("kernelbase!LoadLibraryA", &offset))) {
				RETURN_IF_FAILED(debugsymbols->GetOffsetByName("kernel32!LoadLibraryA", &offset));
			}

			ULONG64 h;
			RETURN_IF_FAILED(_pDebugSystemObjects->GetCurrentProcessHandle(&h));
			auto hProcess{ reinterpret_cast<HANDLE>(h) };

			_remotebuffer.reset(reinterpret_cast<PBYTE>(::VirtualAllocEx(hProcess, nullptr, dll_name.size() + 1, MEM_COMMIT, PAGE_EXECUTE_READWRITE)));
			RETURN_LAST_ERROR_IF_NULL(_remotebuffer.get());

			SIZE_T n;
			RETURN_IF_WIN32_BOOL_FALSE(WriteProcessMemory(hProcess, _remotebuffer.get(), dll_name.c_str(), dll_name.size() + 1, &n));
			if (n != dll_name.size() + 1) {
				return E_FAIL;
			}

			wil::unique_handle hthread{ ::CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)offset, _remotebuffer.get(), 0, &m_remoteThreadId)};

			RETURN_IF_FAILED(_pDebugClient->SetEventCallbacks(this));
			RETURN_IF_FAILED(debugcontrol->Execute(DEBUG_OUTPUT_NORMAL, "g", DEBUG_EXECUTE_NOT_LOGGED));

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
			RETURN_IF_FAILED(_pDebugSystemObjects->GetCurrentThreadSystemId(&threadId));
			if (threadId == m_remoteThreadId) {
				// calls Release and removes the component
				_pDebugClient->SetEventCallbacks(nullptr);
			}
			return DEBUG_STATUS_NO_CHANGE;
		}
	};
}


extern "C" HRESULT CALLBACK injectdll(IDebugClient * debugclient, PCSTR args) {
	wil::com_ptr_t <IDebugControl> pDebugControl{};
	RETURN_IF_FAILED(debugclient->QueryInterface(__uuidof(IDebugControl), reinterpret_cast<PVOID*>(pDebugControl.put())));

	std::string path{ args };
	if (path.empty()) {
		pDebugControl->Output(DEBUG_OUTPUT_ERROR, "DLL path is required.\n");
		return E_INVALIDARG;
	}

	IDebugClient* ndebugclient;
	RETURN_IF_FAILED(debugclient->CreateClient(&ndebugclient));
	try {
		wil::com_ptr_t<lldext::InjectionControl> injection_engine{ new lldext::InjectionControl(ndebugclient) };
		return injection_engine->Inject(args);
	} catch (...) {
		RETURN_CAUGHT_EXCEPTION();
	}
}
