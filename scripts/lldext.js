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
        new host.functionAlias(dbgExec, "dbgExec"),
        new host.functionAlias(dbgExecAndPrint, "dbgExecAndPrint"),
        new host.functionAlias(createSearchEventContext, "eventContext"),
        new host.functionAlias(forEachEvent, "forEachEvent"),
        new host.functionAlias(findEvents, "findEvents"),
    ];
}

// ---------------------------------------------------------------------
// Some helper private methods
// ---------------------------------------------------------------------

const VERBOSE = 0, INFO = 1, ERROR = 2, NONE = 10;

let logLevel = INFO;

function __print(s) {
    host.diagnostics.debugLog(s);
}

function setLogLevel(lvl) {
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

// I need this function as time position objects does not seem to be comparable in JS
function __timePositionString(p) {
    const [seq, steps] = p.toString().split(":", 2);
    return `${seq.padStart(8, "0")}:${steps.padStart(8, "0")}`
}

function __joinStringList(strings, separator) {
}

// ---------------------------------------------------------------------
// Helper functions to work with the debugger
// ---------------------------------------------------------------------

function dbgExec(s) {
    try {
        return host.namespace.Debugger.Utility.Control.ExecuteCommand(s);
    } catch (e) {
        throw new Error(`failed to execute command: ${s}`);
    }
}

function dbgExecAndPrint(s) {
    try {
        for (const line of host.namespace.Debugger.Utility.Control.ExecuteCommand(s)) {
            __print(line + "\n");
        }
    } catch (e) {
        (`failed to execute command: ${s}`);
    }
}

// ---------------------------------------------------------------------
// Helper functions to work with the debugging events
// ---------------------------------------------------------------------

class SearchEventContext {
    constructor(eventDefition, startPosition, endPosition) {
        const [eventType, eventValue] = eventDefition.split(":", 2);

        const isNumber = (str) => {
            const hexRegex = /^(0x|0X)?[0-9A-Fa-f]+$/;
            return hexRegex.test(str);
        }

        const getEventFilter = () => {
            if (eventType === "ld" || eventType === "ul") {
                const moduleOperation = eventType === "ld" ? "ModuleLoaded" : "ModuleUnloaded";
                if (eventValue) {
                    const moduleName = eventValue.toUpperCase();
                    return (ev) => ev.Type === moduleOperation && ev.Module.Name.toUpperCase().endsWith(moduleName);
                }
                return (ev) => ev.Type === moduleOperation;
            }

            if (eventType === "ct" || eventType === "et") {
                const threadOperation = eventType === "ct" ? "ThreadCreated" : "ThreadTerminated";
                if (Number.isInteger(eventValue)) {
                    const threadId = Number.parseInt(eventValue);
                    filter = (ev) => ev.Type === threadOperation && ev.Thread.UniqueId === threadId;
                } else {
                    filter = (ev) => ev.Type === threadOperation;
                }
            }

            if (isNumber(eventValue)) {
                return (ev) => ev.Type === eventType && ev.ExceptionCode === Number.parseInt(eventValue);
            }

            return (ev) => ev.Type === "Exception";
        }

        const startPos = __timePositionString(startPosition === undefined ? host.currentProcess.TTD.Lifetime.MinPosition : startPosition);
        const endPos = __timePositionString(endPosition === undefined ? host.currentProcess.TTD.Lifetime.MaxPosition : endPosition);

        if (eventType === "call") {
            this.__events = host.currentSession.TTD.Calls(eventValue).Where(
                call => __timePositionString(call.TimeStart) >= startPos && __timePositionString(call.TimeEnd) <= endPos);
        } else {
            this.__events = host.currentProcess.TTD.Events.Where(getEventFilter()).Where(
                ev => {
                    const pos = __timePositionString(ev.Position);
                    return pos >= startPos && pos <= endPos;
                });
        }

        this.__eventKind = eventType;
        this.__eventsCount = this.__events.Count();
        this.__currentEventIndex = undefined;
    }

    get currentEvent() {
        if (this.__currentEventIndex === undefined) {
            throw new Error("No current event selected");
        }
        return this.__events.Skip(this.__currentEventIndex).First();
    }

    set currentEventIndex(index) {
        if (index >= 0 && index < this.__eventsCount) {
            this.__currentEventIndex = index;
        } else {
            throw new Error(`Invalid event index: ${index}`);
        }
    }

    get events() { return this.__events; }

    get eventsCount() {
        return this.__eventsCount;
    }

    __seekCurrentEvent() {
        // assert(this.__currentEventIndex !== undefined, "No current event selected");
        (this.__eventKind === "call" ? this.currentEvent.TimeStart : this.currentEvent.Position).SeekTo();
    }

    seekNextEvent() {
        if (this.__currentEventIndex === undefined && this.__eventsCount === 0) {
            return false;
        }

        if (this.__currentEventIndex && this.__currentEventIndex === this.eventsCount - 1) {
            return false;
        }
        this.__currentEventIndex = this.__currentEventIndex === undefined ? 0 : this.__currentEventIndex + 1;
        this.__seekCurrentEvent();
        return true;
    }

    seekPreviousEvent() {
        if (!this.__currentEventIndex) {
            return false;
        }
        this.__currentEventIndex = this.__currentEventIndex - 1;
        this.__seekCurrentEvent();
        return true;
    }
}

function createSearchEventContext(eventDefition, startPosition, endPosition) {
    return new SearchEventContext(eventDefition, startPosition, endPosition);
}

function findEvents(eventDefition, startPosition, endPosition) {
    return new SearchEventContext(eventDefition, startPosition, endPosition).events;
}

function forEachEvent(eventDefition, action, startPosition, endPosition) {
    const eventCtx = new SearchEventContext(eventDefition, startPosition, endPosition);
    while (eventCtx.seekNextEvent()) {
        action(eventCtx.currentEvent);
    }
}

// ---------------------------------------------------------------------
// Helper functions to work with calls recorded in the TTD log
// ---------------------------------------------------------------------

function callstats(functionNameOrAddress) {
    return host.currentSession.TTD.Calls(functionNameOrAddress)
        .GroupBy(c => c.Function === "" ? c.FunctionAddress.toString(16) : c.Function)
        .Select(g => {
            return {
                Function: g.First().Function === "" ? g.First().FunctionAddress.toString(16) : g.First().Function,
                Count: g.Count()
            };
        });
}

// Sometimes WinDbg incorrectly decodes the paramters as 64-bit values instead of 32-bit.
// This function converts the parametrs back to 32-bit values.
function params32(params64) {
    return params64.SelectMany(p => [p.getLowPart(), p.getHighPart()]);
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
