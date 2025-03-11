LLDEXT - WinDbg helpers
=======================

<!-- MarkdownTOC -->

- [Tutorials](#tutorials)
- [Native extension \(lldext.dll\)](#native-extension-lldextdll)
    - [!injectdll dllPath](#injectdll-dllpath)
- [Generic helper functions \(lldext.js\)](#generic-helper-functions-lldextjs)
    - [dbgEval\(exp\)](#dbgevalexp)
    - [dbgExec\(cmd\)](#dbgexeccmd)
    - [dbgExecAndPrint\(cmd\)](#dbgexecandprintcmd)
    - [findAndPrintFunctionCalls\(srcFuncAddr, destFuncAddr, maxDepth\)](#findandprintfunctioncallssrcfuncaddr-destfuncaddr-maxdepth)
    - [findFunctionCalls\(srcFuncAddr, destFuncAddr, maxDepth\)](#findfunctioncallssrcfuncaddr-destfuncaddr-maxdepth)
    - [params32\(params64\)](#params32params64)
    - [range\(start, end, step\)](#rangestart-end-step)
    - [readString/readWideString\(address, length\)](#readstringreadwidestringaddress-length)
- [TTD helper functions \(lldext.js\)](#ttd-helper-functions-lldextjs)
    - [calls\(functionNameOrAddress\)](#callsfunctionnameoraddress)
    - [callstacks\(functionNameOrAddress\)](#callstacksfunctionnameoraddress)
    - [callstats\(calls\)](#callstatscalls)
    - [jumpTo\(timePosition\)](#jumptotimeposition)
    - [seekAndGet\(objects, getTimePosition, func\)](#seekandgetobjects-gettimeposition-func)
    - [seekTimeRangeAndGet\(objects, getStartTimePosition, getEndTimePosition, funcAtStart, funcAtEnd\)](#seektimerangeandgetobjects-getstarttimeposition-getendtimeposition-funcatstart-funcatend)
    - [timePos\(timePosition\)](#timepostimeposition)
- [Functions to work with native controls/windows \(windowing.js\)](#functions-to-work-with-native-controlswindows-windowingjs)
    - [findWindow\(hwnd\)](#findwindowhwnd)
    - [loadSpyxxTree\(path\)](#loadspyxxtreepath)
    - [loadSystemInformerTree\(path\)](#loadsysteminformertreepath)

<!-- /MarkdownTOC -->

Tutorials
---------

My materials covering WinDbg and lldext:

- [WinDbg usage guide](https://wtrace.net/guides/windbg/) at <https://wtrace.net>
- :movie_camera: [Debugging user32 window functions in WinDbg with JavaScript automation](https://youtu.be/lupFi5n7iJk?feature=shared)

Native extension (lldext.dll)
-----------------------------

### !injectdll dllPath

Injects DLL into the debuggee

Current version can be found under the release tab - use version corresponding to the bitness of the debuggee.

Generic helper functions (lldext.js)
-----------------------------

### dbgEval(exp)

Evaluates a debugger expression. Example usage:

```shell
dx @$dbgEval("sizeof(void*)")
# @$dbgEval("sizeof(void*)") : 0x4
```

### dbgExec(cmd)

Executes a command in the debugger and returns a list of strings which compose the command's output.

```shell
dx @$dbgExec("r eax")

# @$dbgExec("r eax")                
#     [0x0]            : eax=0019715c
```

### dbgExecAndPrint(cmd)

Executes a command in the debugger and prints the output to the debugger command window.

```shell
dx @$dbgExecAndPrint("r eax")

# eax=0019715c
# @$dbgExecAndPrint("r eax")
```

### findAndPrintFunctionCalls(srcFuncAddr, destFuncAddr, maxDepth)

*The maxDepth parameter is optional and defaults to 5.*

Similarly to findFunctionCalls finds call paths between two function, but instead of returning them, it prints the call paths in the output:

```shell
dx @$findAndPrintFunctionCalls(0x7ffe4f379e40, 0x7ffe520f0c60, 3)
#
# Found calls to (00007ffe`520f0c60)   ntdll!NtCreateFile:
# |- (00007ffe`4f379fc0)   KERNELBASE!CreateFileInternal+0x5f8
# | |- (00007ffe`4f379e40)   KERNELBASE!CreateFileW+0x77
# |- (00007ffe`4f379fc0)   KERNELBASE!CreateFileInternal+0x589
# | |- (00007ffe`4f379e40)   KERNELBASE!CreateFileW+0x77
```

### findFunctionCalls(srcFuncAddr, destFuncAddr, maxDepth)

*The maxDepth parameter is optional and defaults to 5.*

Analyzes assembly code to find call paths between two functions. The **srcFuncAddr** parameter is the memory address of the source function to analyze, and the **destFuncAddr** is a memory address of the target function. The **maxDepth** defines the maximum call path length. It returns an array of call paths, each containnig an array of call addresses.

An example execution, looking for NtCreateFile call paths from CreateFileW:

```shell
x kernelbase!CreateFileW
# 00007ffe`4f379e40 KERNELBASE!CreateFileW (CreateFileW)

x ntdll!NtCreateFile
# 00007ffe`520f0c60 ntdll!NtCreateFile (NtCreateFile)

dx @$findFunctionCalls(0x7ffe4f379e40, 0x7ffe520f0c60, 3)
# ...
#     length           : 0x2
#     [0x0]            : [(00007ffe`4f379fc0)   KERNELBASE!CreateFileInternal+0x5f8,(00007ffe`4f379e40)   KERNELBASE!CreateFileW+0x77]
#     [0x1]            : [(00007ffe`4f379fc0)   KERNELBASE!CreateFileInternal+0x589,(00007ffe`4f379e40)   KERNELBASE!CreateFileW+0x77]
```

### params32(params64)

This function might be useful if WinDbg incorrectly decodes the parameters as 64-bit integers while the target is 32-bit. It sporadically happens when we lack private symbols. Below is an example of code when the debugger incorrectly discovered the TTD trace bitness and I needed to fix it for both Parameters and ReturnValue proprties:

```shell
dx -g @$seekAndGet(@$cursession.TTD.Calls(0x645a0f01), ev => ev.TimeStart, ev => new { Time = ev.TimeStart, Result = (void *)(int)ev.ReturnValue, Data = *(wchar_t **)(@$params32(ev.Parameters)[1] + 4) + 4 })
```

### range(start, end, step)

Generates a sequence of numbers starting from *start* up to *end* with a step size of *step* (one by default). Example usage:

```shell
dx @$curprocess.TTD.Lifetime
# @$curprocess.TTD.Lifetime                 : [FC:0, 33619E:0]
#     MinPosition      : FC:0 [Time Travel]
#     MaxPosition      : 33619E:0 [Time Travel]

# dump managed stack for TTD trace positions, stepping by 10000 sequence numbers
dx -r2 @$seekAndGet(@$range(0xfc, 0x33619E, 10000), seq => @$create("Debugger.Models.TTD.Position", seq, 0), seq => @$dbgExec("!clrstack"))
```

### readString/readWideString(address, length)

*The length parameter is optional and defaults to undefined.*

Reads an ANSI/UNICODE string from the specific address in the memory.

```shell
dx -g @$seekAndGet(@$cursession.TTD.Calls("jvm!SystemDictionary::resolve_instance_class_or_null").Skip(0x8a0), c => c.TimeStart, c => new { TimeStart = c.TimeStart, ClassName = @$readString(c.Parameters.class_name->_body, c.Parameters.class_name->_length) })

# ======================================================================================
# =           = (+) TimeStart = ClassName                                              =
# ======================================================================================
# = [0x0]     - 69C72:269     - java/util/concurrent/CopyOnWriteArrayList              =
# = [0x1]     - 6A31D:4F2     - java/lang/Object                                       =
# = [0x2]     - 6A333:A1A     - java/util/concurrent/CopyOnWriteArrayList              =
```

TTD helper functions (lldext.js)
--------------------------------

### calls(functionNameOrAddress)

It is basically an alias for `@$currentSession.TTD.Calls`. Example usage:

```shell
dx -g @$calls("advapi32!RegQueryValueEx*").Take(2).Select(c => new { TimeStart = c.TimeStart, Function = c.Function, hKey = c.Parameters[0] })

# ===========================================================================
# =          = (+) TimeStart = (+) Function                     = hKey      =
# ===========================================================================
# = [0x0]    - 633:EDA       - ADVAPI32!RegQueryValueExWStub    - 0x181c    =
# = [0x1]    - 64E:1A99      - ADVAPI32!RegQueryValueExWStub    - 0x1dd4    =
# ===========================================================================
```

### callstacks(functionNameOrAddress)

It dumps a tree of callstacks that triggered a given function. Might be very slow for frequently called functions or when analysing long traces. Example usage:

```shell
dx @$callstacks("kernelbase!CreateFileW").print()

# KERNELBASE!CreateFileW (0x7ff91b884990)
#   |- ucrtbase!_wsopen_nolock + 0xe3 (0x7ff91b369527)
#     |- ucrtbase!common_sopen_dispatch<wchar_t> + 0x65 (0x7ff91b377ed1)
#         ...
#         |- hostfxr!runtime_config_t::parse + 0x1bf (0x7ff910e3d93f)
#           |- hostfxr!fx_definition_t::parse_runtime_config + 0x17 (0x7ff910e1c180)
#             |- hostfxr!`anonymous namespace'::read_config + 0x1b0 (0x7ff910e1c180)
#               |- hostfxr!`anonymous namespace'::get_init_info_for_app + 0xa69 (0x7ff910e1d1d9)
#                 |- hostfxr!`anonymous namespace'::read_config_and_execute + 0x7e (0x7ff910e1e03e)
#                   |- hostfxr!fx_muxer_t::handle_exec_host_command + 0x16c (0x7ff910e202ec)
#                     |- hostfxr!fx_muxer_t::execute + 0x494 (0x7ff910e1e644)
#                       |- hostfxr!hostfxr_main_startupinfo + 0xa0 (0x7ff910e185a0)
#                         |- throwexc!exe_start + 0x878 (0x7ff6bcb7f998)
#                           |- throwexc!wmain + 0x146 (0x7ff6bcb7fda6)
#                             |- throwexc!invoke_main + 0x22 (0x7ff6bcb812e8)
#                               |- throwexc!__scrt_common_main_seh + 0x10c (0x7ff6bcb812e8)
#                                 |- KERNEL32!BaseThreadInitThunk + 0x1d (0x7ff91da3257d)
#                                   |- ntdll!RtlUserThreadStart + 0x28 (0x7ff91e08aa48)
#           |- hostfxr!fx_definition_t::parse_runtime_config + 0x1f (0x7ff910e25760)
#             |- hostfxr!fx_resolver_t::read_framework + 0x440 (0x7ff910e25760)
#               |- hostfxr!fx_resolver_t::resolve_frameworks_for_app + 0x127 (0x7ff910e26037)
#                 |- hostfxr!`anonymous namespace'::get_init_info_for_app + 0xded (0x7ff910e1d55d)
#                   |- hostfxr!`anonymous namespace'::read_config_and_execute + 0x7e (0x7ff910e1e03e)
#                     |- hostfxr!fx_muxer_t::handle_exec_host_command + 0x16c (0x7ff910e202ec)
#                       |- hostfxr!fx_muxer_t::execute + 0x494 (0x7ff910e1e644)
#                         |- hostfxr!hostfxr_main_startupinfo + 0xa0 (0x7ff910e185a0)
#                           |- throwexc!exe_start + 0x878 (0x7ff6bcb7f998)
#                             |- throwexc!wmain + 0x146 (0x7ff6bcb7fda6)
#                               |- throwexc!invoke_main + 0x22 (0x7ff6bcb812e8)
#                                 |- throwexc!__scrt_common_main_seh + 0x10c (0x7ff6bcb812e8)
#                                   |- KERNEL32!BaseThreadInitThunk + 0x1d (0x7ff91da3257d)
#                                     |- ntdll!RtlUserThreadStart + 0x28 (0x7ff91e08aa48)
```

### callstats(calls)

It prints stats about calls of a given function or functions (wildcards and function addresses are supported), for example:

```shell
dx -g @$callstats(@$calls("kernelbase!*File*"))

# ==============================================================================================================================
# =                                                              = Function                                          = Count   =
# ==============================================================================================================================
# = ["KERNELBASE!GetModuleFileNameW"] : [object Object]          - KERNELBASE!GetModuleFileNameW                     - 0x13    =
# = ["KERNELBASE!GetFileAttributesExW"] : [object Object]        - KERNELBASE!GetFileAttributesExW                   - 0x23    =
# = ["KERNELBASE!IsBrokeredSetFileAttributesWPresent"] : [obj... - KERNELBASE!IsBrokeredSetFileAttributesWPresent    - 0x8     =
# = ["KERNELBASE!FindFirstFileExW"] : [object Object]            - KERNELBASE!FindFirstFileExW                       - 0x2     =
# = ["KERNELBASE!InternalFindFirstFileExW"] : [object Object]    - KERNELBASE!InternalFindFirstFileExW               - 0x2     =
# = ["KERNELBASE!BasepInitializeFindFileHandle"] : [object Ob... - KERNELBASE!BasepInitializeFindFileHandle          - 0x2     =
# = ["KERNELBASE!CloseEncryptedFileRaw"] : [object Object]       - KERNELBASE!CloseEncryptedFileRaw                  - 0x17    =
# = ["KERNELBASE!GetFileType"] : [object Object]                 - KERNELBASE!GetFileType                            - 0x6     =
# = ["KERNELBASE!ReadFile"] : [object Object]                    - KERNELBASE!ReadFile                               - 0x1f    =
# = ["KERNELBASE!PathCchRemoveFileSpec"] : [object Object]       - KERNELBASE!PathCchRemoveFileSpec                  - 0x1     =
# = ["KERNELBASE!CreateFileMappingNumaW"] : [object Object]      - KERNELBASE!CreateFileMappingNumaW                 - 0x9     =
# = ["KERNELBASE!MapViewOfFileExNuma"] : [object Object]         - KERNELBASE!MapViewOfFileExNuma                    - 0x53    =
# = ["KERNELBASE!UnmapViewOfFile"] : [object Object]             - KERNELBASE!UnmapViewOfFile                        - 0x24    =
# = ["KERNELBASE!WriteFile"] : [object Object]                   - KERNELBASE!WriteFile                              - 0x9     =
# = ["KERNELBASE!BasepLoadLibraryAsDataFileInternal"] : [obje... - KERNELBASE!BasepLoadLibraryAsDataFileInternal     - 0x5     =
# = ["KERNELBASE!BasepReleaseDataFileHandle"] : [object Object]  - KERNELBASE!BasepReleaseDataFileHandle             - 0x5     =
# ==============================================================================================================================

dx -g @$callstats(@$cursession.TTD.Calls("advapi32!Reg*").Where(c => c.TimeStart >= @$timePos("26E269:225B") && c.TimeEnd <= @$timePos("26F4DE:80E")))
# ====================================================================================================
# =                                                     = Function                         = Count   =
# ====================================================================================================
# = ["ADVAPI32!RegOpenKeyExWStub"] : [object Object]    - ADVAPI32!RegOpenKeyExWStub       - 0x16    =
# = ["ADVAPI32!RegCloseKeyStub"] : [object Object]      - ADVAPI32!RegCloseKeyStub         - 0x8     =
# = ["ADVAPI32!RegQueryValueExWStub"] : [object Object] - ADVAPI32!RegQueryValueExWStub    - 0x3     =
# ====================================================================================================
```

### jumpTo(timePosition)

Jumps to the time position in a **TTD trace**. Example usage:

```shell
dx @$jumpTo("6DDA9:B6C")

# (1d88.190c): Break instruction exception - code 80000003 (first/second chance not available)
# Time Travel Position: 6DDA9:B6C
# @$jumpTo("6DDA9:B6C")
```

### seekAndGet(objects, getTimePosition, func)

It executes a given function (func) for each object after setting the time position in the TTD trace. It returns the results of the function call. Example usages:

```shell
dx -r3 @$seekAndGet(@$cursession.TTD.Memory(0x16a078c0, 0x16a078c4, "w"), m => m.TimeStart, m => new { OldValue = m.OverwrittenValue, NewValue = m.Value, Stack = @$curstack.Frames })

dx -r3 @$seekAndGet(@$cursession.TTD.Memory(0x16a078c0, 0x16a078c4, "w"), m => m.TimeStart, m => new { OldValue = m.OverwrittenValue, NewValue = m.Value, Stack = @$dbgExec("k") })

# group calls to outerHTML property by a given COM class instance
dx -g @$seekAndGet(@$cursession.TTD.Calls("mshtml!CElement::put_outerHTML"), c => c.TimeStart, c => new { TimeStart = c.TimeStart, Class = **(void ***)(@$curthread.Registers.User.esp + 4) }).GroupBy(t => t.Class).Select(g => new { Class = g.Last().Class, LastCall = g.Last().TimeStart, Count = g.Count() })
```

### seekTimeRangeAndGet(objects, getStartTimePosition, getEndTimePosition, funcAtStart, funcAtEnd)

For each object in the objects list, it seeks the start time position and executes funcAtStart, saving its result. Then it seeks the end time position and executes the funcAtEnd function. This function is useful when, for example, a function returns a result in an output parameter. We can save the parameter address in the context returned from the funcAtStart and use it in funcAtEnd. Example usage:

```shell
dx -g @$seekTimeRangeAndGet(@$cursession.TTD.Calls("ADVAPI32!RegOpenKeyExWStub").Take(2), c => c.TimeStart, c => c.TimeEnd, c => new { phkResultAddr = *(void ***)(@esp + 0x14) }, (c, ctx) => new { TimeStart = c.TimeStart, Result = (void *)(int)c.ReturnValue, hKey = @$params32(c.Parameters)[0], lpSubKey = (wchar_t *)@$params32(c.Parameters)[1], phkResult = c.ReturnValue == 0 ? *ctx.phkResultAddr : 0 })

# ================================================================================================================================
# =          = (+) TimeStart = Result = hKey          = (+) lpSubKey                                                 = phkResult =
# ================================================================================================================================
# = [0x0]    - 2E4:70D       - 0x2    - 0x304         - 0x3a8c970 : "Policies\Microsoft\Office\16.0\Common\Roaming"  - 0         =
# = [0x1]    - 36E:534       - 0x5    - 0x30c         - 0x3a8c998 : "16.0\Common\Roaming"                            - 0         =
# ================================================================================================================================
```

### timePos(timePosition)

Creates a time position object (shorter version of `@$create("Debugger.Models.TTD.Position", ?, ?)`). Example usage:

```shell
dx @$timePos("B6B47:48")

# @$tt("B6B47:48")                 : B6B47:48 [Time Travel]
#     Sequence         : 0xb6b47
#     Steps            : 0x48
#     SeekTo           [Method which seeks to time position]
#     ToSystemTime     [Method which obtains the approximate system time at a given position]

dx -g @$cursession.TTD.Memory(0x6da752c8, 0x6da752c8 + 10 * 4, "r").OrderBy(m => m.TimeStart).Where(m => m.TimeStart >= @$timePos("B6AD3:270")).Select(m => new { TimeStart = m.TimeStart, TimeEnd = m.TimeEnd, Value = m.Value, Index = (int)(m.Address - 0x6da752c8) / 4 })

# =====================================================================
# =           = (+) TimeStart = (+) TimeEnd   = (+) Value     = Index =
# =====================================================================
# = [0x7]     - B6B47:48      - B6B47:48      - 0x6d8a52b0    - 6     =
# = [0x8]     - B6D73:D3      - B6D73:D3      - 0x6d8a52b0    - 6     =
# = [0x9]     - B6E0A:1BD     - B6E0A:1BD     - 0x6d8a52b0    - 6     =
# = [0xa]     - B6E67:7EF     - B6E67:7EF     - 0x6d8a52b0    - 6     =
```

Functions to work with native controls/windows (windowing.js)
-------------------------------------------------------------

### findWindow(hwnd)

Retrieves information about a window from a previously loaded Spyxx tree. Example usages:

```shell
.scriptload windowing.js

dx -g @$cursession.TTD.Calls("patcher!hooked_SetWindowPos").Select(c => new { HWND = c.Parameters.window_handle, Class = @$scriptContents.findWindow(c.Parameters.window_handle).className, Cx = c.Parameters.cx, Cy = c.Parameters.cy, TimeStart = c.TimeStart, Time = c.SystemTimeStart })
# ================================================================================================================================================
# =                    = (+) HWND    = Class                       = Cx      = Cy     = (+) TimeStart = (+) Time                                 =
# ================================================================================================================================================
# = [0x0]              - 0x160046    - tooltips_class32            - 1536    - 813    - 676:55        - Wednesday, March 6, 2024 14:14:15.318    =
# = [0x1]              - 0x160046    - tooltips_class32            - 0       - 0      - 682:55        - Wednesday, March 6, 2024 14:14:15.318    =
# = [0x2]              - 0x160046    - tooltips_class32            - 0       - 0      - 2333:253      - Wednesday, March 6, 2024 14:14:19.662    =
...
``` 

```shell
bp user32!NtUserSetWindowPos "dx new { function = \"SetWindowPos\", hWnd = (void *)@rcx, class = @$findWindow(@rcx).className, hWndInsertAfter = (void *)@rdx, X = (int)@r8, Y = (int)@r9, cx = *(int *)(@rsp+0x28), cy = *(int *)(@rsp+0x30), uFlags = *(unsigned int *)(@rsp+0x38) }; g"
```

### loadSpyxxTree(path)

Parses the window tree (typically in from an sxt file) saved by Spyxx tool. Example usage:

```shell
.scriptload windowing.js

dx @$scriptContents.loadSpyxxTree("C:\\temp\\windows.sxt")
```

Example file content:

```
Window 00010010 "" #32769 (Desktop)
    Window 00010100 "" Worker Window
    Window 00010364 "Default IME" IME
    Window 00010362 "Microsoft Text Input Application" Windows.UI.Core.CoreWindow
    Window 00010380 "Default IME" IME
    Window 0001037E "" ApplicationFrameWindow
```

### loadSystemInformerTree(path)

Parses the window tree copied from the System Informer Windows tab for a given process. Example usage:

```shell
.scriptload windowing.js

dx @$scriptContents.loadSpyxxTree("C:\\temp\\windows.txt")
```

Example file content:

```
UserAdapterWindowClass, 0x605d4, , iexplore.exe (18168): unnamed thread (16852), CoreMessaging.dll
CicMarshalWndClass, 0x120442, CicMarshalWnd, iexplore.exe (18168): unnamed thread (16852), msctf.dll
Isolation Thread Message Window, 0xa01b8, , iexplore.exe (18168): unnamed thread (2824), iexplore.exe
Isolation Thread Message Window, 0x3068c, , iexplore.exe (18168): unnamed thread (16852), iexplore.exe
OleMainThreadWndClass, 0x805b6, OleMainThreadWndName, iexplore.exe (18168): unnamed thread (16852), combase.dll
```
