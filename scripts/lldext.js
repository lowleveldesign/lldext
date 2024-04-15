/// <reference path="JSProvider.d.ts" />

"use strict";

/*
.scriptunload lldext.js; .scriptload lldext.js
*/

function initializeScript() {
    return [
        new host.apiVersionSupport(1, 7),
        new host.functionAlias(callstacks, "callstacks"),
        new host.functionAlias(callstats, "callstats"),
        new host.functionAlias(params32, "params32"),
        new host.functionAlias(exceptions, "exceptions"),
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

function __exec_cmd(s) {
    try {
        return host.namespace.Debugger.Utility.Control.ExecuteCommand(s);
    } catch (e) {
        throw new Error(`failed to execute command: ${s}`);
    }
}

// ---------------------------------------------------------------------
// Helper functions to work with calls recorded in the TTD log
// ---------------------------------------------------------------------

function callstats(functionNameOrAddress) {
    return host.currentSession.TTD.Calls(functionNameOrAddress).GroupBy(c => c.Function)
        .Select(g => { return { Function: g.First().Function, Count: g.Count() }; });
}

// Sometimes WinDbg incorrectly decodes the paramters as 64-bit values instead of 32-bit.
// This function converts the parametrs back to 32-bit values.
function params32(params64) {
    return params64.SelectMany(p => [p.getLowPart(), p.getHighPart()]);
}

function exceptions() {
    return host.currentProcess.TTD.Events.Where(ev => ev.Type === "Exception");
}

function forEachTimePosition(objects, getTimePosition, action) {
    const results = new Map(); // Map<TTDTimePosition, T>
    for (const obj of objects) {
        const timePosition = getTimePosition(obj);
        timePosition.SeekTo();
        results.push(action(obj));
    }
    return results;
}

// ---------------------------------------------------------------------
// The callstacks function scans all the calls of a certain function
// and builds a tree of all possible call stacks.
// ---------------------------------------------------------------------

class StackNode {

    constructor(address, resolvedAddress) {
        this.__address = address;
        this.__resolvedAddress = resolvedAddress;
        this.__parents = new Map(); // Map<address, StackNode>

        this.__indent = "  ";
    }

    index(frames) {
        if (!frames.Any(_ => true)) {
            return;
        }
        const functionFrame = frames.First();
        const functionAddress = functionFrame.Attributes.InstructionOffset;
        if (!this.__parents.has(functionAddress)) {
            const parentNode = new StackNode(functionAddress, functionFrame.toString())
            this.__parents.set(functionAddress, parentNode);
        }
        this.__parents.get(functionAddress).index(frames.Skip(1));
    }

    __printNode(nodeIndent) {
        let indent = this.__indent.repeat(nodeIndent);
        let s = `${indent}|- ${this.__resolvedAddress} (${this.__address})`;
        for (const [addr, node] of this.__parents) {
            s += `\r\n${node.__printNode(nodeIndent + 1)}`;
        }
        return s;
    }

    toString() {
        return `${this.__resolvedAddress} (${this.__address})`;
    }

    print() {
        let s = `${this.__resolvedAddress} (${this.__address})`;
        for (const [addr, node] of this.__parents) {
            s += `\r\n${node.__printNode(1)}`;
        }
        __logn(INFO, s);
    }

    get address() { return this.__address };

    get functionName() { return this.__resolvedAddress; }

    get parentNodes() { return this.__parents.values(); }
}

function callstacks(functionNameOrAddress) {
    const callsTimes = host.currentSession.TTD.Calls(functionNameOrAddress)
        .Select(c => c.TimeStart);

    if (callsTimes.length === 0) {
        return undefined;
    }

    callsTimes.First().SeekTo();
    const functionFrame = host.currentThread.Stack.Frames[0];
    const functionAddress = functionFrame.Attributes.InstructionOffset;

    const stackNode = new StackNode(functionAddress, functionFrame.toString());

    for (const callTime of callsTimes) {
        callTime.SeekTo();
        stackNode.index(host.currentThread.Stack.Frames.Skip(1));
    }

    return stackNode;
}
