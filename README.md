LLDEXT - WinDbg helpers
=======================

<!-- MarkdownTOC -->

- [Tutorials](#tutorials)
- [Native extension \(lldext.dll\)](#native-extension-lldextdll)
    - [!injectdll dllPath](#injectdll-dllpath)
- [Helper functions \(lldext.js\)](#helper-functions-lldextjs)
    - [readString/readWideString\(address, length\)](#readstringreadwidestringaddress-length)
    - [params32\(params64\)](#params32params64)
    - [dbgExec\(cmd\)](#dbgexeccmd)
    - [dbgExecAndPrint\(cmd\)](#dbgexecandprintcmd)
    - [findFunctionCalls\(srcFuncAddr, destFuncAddr, maxDepth\)](#findfunctioncallssrcfuncaddr-destfuncaddr-maxdepth)
    - [findAndPrintFunctionCalls\(srcFuncAddr, destFuncAddr, maxDepth\)](#findandprintfunctioncallssrcfuncaddr-destfuncaddr-maxdepth)
    - [callstats\(functionNameOrAddress\)](#callstatsfunctionnameoraddress)
    - [callstacks\(functionNameOrAddress\)](#callstacksfunctionnameoraddress)
    - [seekAndGet\(objects, getTimePosition, func\)](#seekandgetobjects-gettimeposition-func)
    - [jumpTo\(timePosition\)](#jumptotimeposition)
    - [timePos\(timePosition\)](#timepostimeposition)
- [Functions helping to recognize native controls/windows \(windowing.js\)](#functions-helping-to-recognize-native-controlswindows-windowingjs)
    - [loadSpyxxTree\(path\)](#loadspyxxtreepath)
    - [loadSystemInformerTree\(path\)](#loadsysteminformertreepath)
    - [findWindow\(hwnd\)](#findwindowhwnd)

<!-- /MarkdownTOC -->

Tutorials
---------

My materials covering WinDbg and lldext:

- [WinDbg usage guide](https://wtrace.net/guides/using-windbg/) at <https://wtrace.net>
- :movie_camera: [Debugging user32 window functions in WinDbg with JavaScript automation](https://youtu.be/lupFi5n7iJk?feature=shared)

Native extension (lldext.dll)
-----------------------------

### !injectdll dllPath

Injects DLL into the debuggee

Current version can be found under the release tab - use version corresponding to the bitness of the debuggee.

Helper functions (lldext.js)
----------------------------

The scripts folder contains JavaScript scripts. The following commands / functions are available:

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

### params32(params64)

This function might be useful if WinDbg incorrectly decodes the parameters as 64-bit integers while the target is 32-bit. It sporadically happens when we lack private symbols.

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

### callstats(functionNameOrAddress)

It works with **TTD traces** and prints stats about calls of a given function or functions (wildcards and function addresses are supported), for example:

```shell
dx -g @$callstats("kernelbase!*File*")

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
```

### callstacks(functionNameOrAddress)

It works with **TTD traces** and dumps a tree of callstacks that triggered a given function. Might be very slow for frequently called functions or when analysing long traces. Example usage:

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

### seekAndGet(objects, getTimePosition, func)

It works with **TTD traces** and executes a given function (func) for each object after setting the time position in the TTD trace. It returns the results of the function call. Example usages:

```shell
dx -r3 @$seekAndGet(@$cursession.TTD.Memory(0x16a078c0, 0x16a078c4, "w"), m => m.TimeStart, m => new { OldValue = m.OverwrittenValue, NewValue = m.Value, Stack = @$curstack.Frames })

dx -r3 @$seekAndGet(@$cursession.TTD.Memory(0x16a078c0, 0x16a078c4, "w"), m => m.TimeStart, m => new { OldValue = m.OverwrittenValue, NewValue = m.Value, Stack = @$dbgExec("k") })

# group calls to outerHTML property by a given COM class instance
dx -g @$seekAndGet(@$cursession.TTD.Calls("mshtml!CElement::put_outerHTML"), c => c.TimeStart, c => new { TimeStart = c.TimeStart, Class = **(void ***)(@$curthread.Registers.User.esp + 4) }).GroupBy(t => t.Class).Select(g => new { Class = g.Last().Class, LastCall = g.Last().TimeStart, Count = g.Count() })
```

### jumpTo(timePosition)

Jumps to the time position in a **TTD trace**. Example usage:

```shell
dx @$jumpTo("6DDA9:B6C")

# (1d88.190c): Break instruction exception - code 80000003 (first/second chance not available)
# Time Travel Position: 6DDA9:B6C
# @$jumpTo("6DDA9:B6C")
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

Functions helping to recognize native controls/windows (windowing.js)
---------------------------------------------------------------------

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