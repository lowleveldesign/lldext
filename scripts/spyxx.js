
/// <reference path="JSProvider.d.ts" />

"use strict";

function initializeScript() {
    return [
        new host.apiVersionSupport(1, 7),
        new host.functionAlias(loadSpyxxTree, "loadSpyxxTree"),
        new host.functionAlias(findWindow, "findWindow"),
    ];
}

// ---------------------------------------------------------------------
// Some helper private methods
// ---------------------------------------------------------------------

const VERBOSE = 0, INFO = 1, ERROR = 2, NONE = 10;

let logLevel = INFO;

function set_logLevel(lvl) {
    logLevel = lvl;
}

function __log(lvl, s) {
    if (lvl >= logLevel) {
        host.diagnostics.debugLog(s);
    }
}

function __logn(lvl, s) {
    __log(lvl, s + "\n")
}

// ---------------------------------------------------------------------
// Spyxx tree parser and window finder
// ---------------------------------------------------------------------

const __windows = new Map(); // Map<hwnd, Window>

class Window {
    constructor(hwnd, title, className) {
        this.hwnd = hwnd;
        this.title = title;
        this.className = className;
    }

    toString() {
        return `${hwnd} "${title}" (${className}`;
    }
}

function loadSpyxxTree(path) {
    const fileSystem = host.namespace.Debugger.Utility.FileSystem;
    if (!fileSystem.FileExists(path)) {
        throw new Error(`File not found: ${path}`);
    }
    const re = /Window\s+([0-9A-F]+)\s+"([^"]*)"\s+(.*)$/;

    const reader = fileSystem.CreateTextReader(path, "Utf16");
    for (const line of reader.ReadLineContents()) {
        const m = re.exec(line);
        if (m) {
            const hwnd = parseInt(m[1], 16);
            const w = new Window(hwnd, m[2], m[3]);
            __windows.set(hwnd, w);
        }
    }
}

function findWindow(hwnd) {
    if (typeof hwnd === "string") {
        hwnd = parseInt(hwnd, 16);
    } else if (hwnd.asNumber) {
        hwnd = hwnd.asNumber();
    } else if (hwnd.address) {
        hwnd = hwnd.address.asNumber();
    }
    return __windows.get(hwnd);
}
