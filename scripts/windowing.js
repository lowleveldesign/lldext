
/// <reference path="JSProvider.d.ts" />

"use strict";

/*
.scriptunload windowing.js; .scriptload windowing.js
*/

function initializeScript() {
    return [
        new host.apiVersionSupport(1, 7),
        new host.functionAlias(loadSpyxxTree, "loadSpyxxTree"),
        new host.functionAlias(loadSystemInformerTree, "loadSystemInformerTree"),
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
// Spyxx and System Informer tree parser and window finder
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

const UnknownWindow = new Window(0x0, undefined, undefined);

function loadSpyxxTree(path) {
    const fileSystem = host.namespace.Debugger.Utility.FileSystem;
    if (!fileSystem.FileExists(path)) {
        throw new Error(`File not found: ${path}`);
    }
    const re = /Window\s+([0-9A-F]+)\s+"([^"]*)"\s+(.*)$/;

    const file = fileSystem.OpenFile(path);
    const reader = fileSystem.CreateTextReader(file, "Utf16");
    for (const line of reader.ReadLineContents()) {
        const m = re.exec(line);
        if (m) {
            const hwnd = parseInt(m[1], 16);
            const w = new Window(hwnd, m[2], m[3]);
            __windows.set(hwnd, w);
        }
    }
    file.Close();
}

function loadSystemInformerTree(path) {
    const fileSystem = host.namespace.Debugger.Utility.FileSystem;
    if (!fileSystem.FileExists(path)) {
        throw new Error(`File not found: ${path}`);
    }

    const file = fileSystem.OpenFile(path);
    const reader = fileSystem.CreateTextReader(file, "Utf8");
    for (const line of reader.ReadLineContents()) {
        const tokens = line.split(",");
        if (tokens => tokens !== null && tokens.length >= 3) {
            const hwnd = parseInt(tokens[1], 16);
            const w = new Window(hwnd, tokens[2] ? tokens[2].trim() : "n/a", tokens[0] ? tokens[0].trim() : "n/a");
            __windows.set(hwnd, w);
        }
    }
    file.Close();
}

function findWindow(hwnd) {
    if (typeof hwnd === "string") {
        hwnd = parseInt(hwnd, 16);
    } else if (typeof hwnd === "object") {
        if ("address" in hwnd) {
            hwnd = hwnd.address.asNumber();
        } else if ("asNumber" in hwnd) {
            hwnd = hwnd.asNumber();
        }
    }
    const w = __windows.get(hwnd);
    return w ? w : UnknownWindow;
}

function printWindows() {
    for (const [hwnd, w] of __windows) {
        __logn(INFO, `${hwnd} "${w.title}" (${w.className})`);
    }
}
