LLDEXT - WinDbg helpers
=======================

My WinDbg extension and automation scripts.

:point_right: If you're working with WinDbg, check out also [my WinDbg guide](https://wtrace.net/guides/using-windbg/). :point_left:

<!-- MarkdownTOC -->

- [Native extension \(lldext.dll\)](#native-extension-lldextdll)
    - [!injectdll](#injectdll)
- [Scripts](#scripts)
    - [callstats \(lldext.js\)](#callstats-lldextjs)
    - [params32 \(lldext.js\)](#params32-lldextjs)
    - [callstacks \(lldext.js\)](#callstacks-lldextjs)
    - [loadSpyxxTree \(spyxx.js\)](#loadspyxxtree-spyxxjs)
    - [findWindow \(spyxx.js\)](#findwindow-spyxxjs)

<!-- /MarkdownTOC -->

Native extension (lldext.dll)
-----------------------------

### !injectdll

Injects DLL into the debuggee

Current version can be found under the release tab - use version corresponding to the bitness of the debuggee.

Scripts
-------

The scripts folder contains JavaScript scripts. The following commands / functions are available:

### callstats (lldext.js)

It works with TTD traces and prints stats about calls of a given function or functions (wildcards and function addresses are supported), for example:

```shell
FIXME
```

### params32 (lldext.js)

This function might be useful if WinDbg incorrectly decodes the parameters as 64-bit integers while the target is 32-bit. It sporadically happens when we lack private symbols. Example usage:

```shell
FIXME
```

### callstacks (lldext.js)

It works with TTD traces and dumps a tree of callstacks that triggered a given function. Might be very slow for frequently called functions or when analysing long traces. Example usage:

```shell
FIXME
```

### loadSpyxxTree (spyxx.js)

Parses the window tree (typically in from an sxt file) saved by Spyxx tool. Example usage:

```shell
.scriptload spyxx.js

dx @$scriptContents.loadSpyxxTree("C:\\temp\\windows.sxt")
```

### findWindow (spyxx.js)

Retrieves information about a window from a previously loaded Spyxx tree. Example usage:

```shell
.scriptload spyxx.js

dx -g @$cursession.TTD.Calls("patcher!hooked_SetWindowPos").Select(c => new { HWND = c.Parameters.window_handle, Class = @$scriptContents.findWindow(c.Parameters.window_handle).className, Cx = c.Parameters.cx, Cy = c.Parameters.cy, TimeStart = c.TimeStart, Time = c.SystemTimeStart })
# ================================================================================================================================================
# =                    = (+) HWND    = Class                       = Cx      = Cy     = (+) TimeStart = (+) Time                                 =
# ================================================================================================================================================
# = [0x0]              - 0x160046    - tooltips_class32            - 1536    - 813    - 676:55        - Wednesday, March 6, 2024 14:14:15.318    =
# = [0x1]              - 0x160046    - tooltips_class32            - 0       - 0      - 682:55        - Wednesday, March 6, 2024 14:14:15.318    =
# = [0x2]              - 0x160046    - tooltips_class32            - 0       - 0      - 2333:253      - Wednesday, March 6, 2024 14:14:19.662    =
...
``` 
