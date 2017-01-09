# LLD extension

My own WinDbg extension :) Contains the following commands:

- `!injectdll <path>` - injects DLL into the debuggee
- `!ufgraph <name-or-address-of-the-function> [<address-in-the-function>]` - runs [ufgraph](https://github.com/bfosterjr/ufgraph) on a given function. The ufgraph.py must be placed in the winext folder of the windbg installation and python must be in the PATH. As stated on the ufgraph page, you need to have Graphviz bin folder in the PATH too.

Current version can be found under the release tab - use version corresponding to the bitness of the debuggee process.

