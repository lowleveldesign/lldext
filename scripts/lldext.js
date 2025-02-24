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
        new host.functionAlias(dbgEval, "dbgEval"),
        new host.functionAlias(dbgExec, "dbgExec"),
        new host.functionAlias(dbgExecAndPrint, "dbgExecAndPrint"),
        new host.functionAlias(seekAndGet, "seekAndGet"),
        new host.functionAlias(jumpTo, "jumpTo"),
        new host.functionAlias(timePos, "timePos"),
        new host.functionAlias(readString, "readString"),
        new host.functionAlias(readWideString, "readWideString"),
        new host.functionAlias(findFunctionCalls, "findFunctionCalls"),
        new host.functionAlias(findAndPrintFunctionCalls, "findAndPrintFunctionCalls"),
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

function __println(s) {
    host.diagnostics.debugLog(`${s}\n`);
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

function __is64bit() {
    return dbgEval("sizeof(void*)") == 8;
}

function __assert(f, msg = "") {
    if (!f) {
        throw new Error(`Assertion error ${msg}`);
    }
}

// ---------------------------------------------------------------------
// Helper functions to work with the debugger
// ---------------------------------------------------------------------

function dbgEval(v) {
    return host.evaluateExpression(v);
}

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
            __println(line);
        }
    } catch (e) {
        (`failed to execute command: ${s}`);
    }
}

function readString(address, length = undefined) {
    return length === undefined ? host.memory.readString(address) : host.memory.readString(address, length);
}

function readWideString(address, length = undefined) {
    return length === undefined ? host.memory.readWideString(address) : host.memory.readWideString(address, length);
}

function __resolveAddr(addr) {
    const rgx = /\([a-f,`,\d]+/;
    return dbgExec(`ln ${addr}`).First(line => rgx.test(line)).split('|')[0].trimEnd();
}

// ---------------------------------------------------------------------
// Helper functions to work with assembly
// ---------------------------------------------------------------------

function findFunctionCalls(srcFuncAddr, destFuncAddr, maxDepth = 5) {
    const srcCallAddr = host.parseInt64(srcFuncAddr, 16); // throws error if srcFuncAddrArg is not a valid Int64 number
    const destCallAddr = host.parseInt64(destFuncAddr, 16); // throws error if destFuncAddrArg is not a valid Int64 number

    const disasm = host.namespace.Debugger.Utility.Code.CreateDisassembler();
    const is64bit = __is64bit();

    function getTargetFunctionAddress(operand) {
        const immediateValue = operand.ImmediateValue;
        if (operand.Attributes.IsMemoryReference) {
            var elemSize = is64bit ? 8 : 4;
            try {
                return host.memory.readMemoryValues(immediateValue, 1, elemSize, false)[0];
            } catch (err) {
                __logn(ERROR, err);
            }
        } else if (operand.Attributes.IsImmediate) {
            return immediateValue;
        }
        return 0;
    }

    let callInstructions = disasm.DisassembleFunction(srcCallAddr).BasicBlocks.SelectMany(b => b.Instructions.Where(inst => inst.Attributes.IsCall));

    const callStack = [];
    for (const callInstruction of callInstructions) {
        var callTargetAddr = getTargetFunctionAddress(callInstruction.Operands[0]);
        if (callTargetAddr) {
            callStack.push([callTargetAddr, callInstruction.Address, 0]);
        } else {
            __logn(INFO, `Skipping call at ${callInstruction.Address} as it's impossible to decode its target.`);
        }
    }

    const foundCallStacks = [];
    function updateFoundCallStacks() {
        const foundCallStack = [];
        let lastDepth = callStack.at(-1)[2] + 1; // depth - +1 to make the first stack to be added
        for (let i = callStack.length - 1; i >= 0 && lastDepth > 0; i--) {
            const [_, callAddr, depth] = callStack[i];
            if (depth < lastDepth) {
                let resolvedAddr = __resolveAddr(callAddr);
                foundCallStack.push(resolvedAddr);
            }
            lastDepth = depth;
        }
        foundCallStacks.push(foundCallStack);
    }

    let lastDepth = 0;
    while (callStack.length > 0) {
        const [funcAddr, _, depth] = callStack.at(-1);
        const backToParent = depth < lastDepth;

        lastDepth = depth;

        if (backToParent) {
            callStack.pop();
            continue;
        }

        if (funcAddr == destCallAddr) {
            updateFoundCallStacks();
            callStack.pop();
            continue;
        }

        if (depth + 1 <= maxDepth) {
            callInstructions = disasm.DisassembleFunction(funcAddr).BasicBlocks.SelectMany(b => b.Instructions.Where(inst => inst.Attributes.IsCall));

            const callStackLength = callStack.length;
            for (const callInstruction of callInstructions) {
                const callTargetAddr = getTargetFunctionAddress(callInstruction.Operands[0]);
                if (callTargetAddr) {
                    callStack.push([callTargetAddr, callInstruction.Address, depth + 1]);
                } else {
                    __logn(INFO, `Skipping call at ${callInstruction.Address} as it's impossible to decode its target.`);
                }
            }
            if (callStackLength == callStack.length) {
                // no new calls added
                callStack.pop();
            }
        } else {
            __logn(VERBOSE, "Max stack depth reached.");
            callStack.pop();
        }
    }

    return foundCallStacks;
}

function findAndPrintFunctionCalls(srcFuncAddr, destFuncAddr, maxDepth = 5) {
    const foundCallStacks = findFunctionCalls(srcFuncAddr, destFuncAddr, maxDepth);
    const resolvedDestCallAddr = __resolveAddr(destFuncAddr);

    __println(`\nFound calls to ${resolvedDestCallAddr}:`);
    for (const callStack of foundCallStacks) {
        for (let i = 0; i < callStack.length; i++) {
            __println(`${'| '.repeat(i)}|- ${callStack[i]}`);
        }
    }

    __println("");
}

// ---------------------------------------------------------------------
// Helper functions to work with the time-based debugging objects
// ---------------------------------------------------------------------

function seekAndGet(objects, getTimePosition, func) {
    const objectsIter = objects[Symbol.iterator]();

    const iter = {
        next() {
            const { value: obj, done } = objectsIter.next();
            if (!done) {
                const timePosition = getTimePosition(obj);
                timePosition.SeekTo();
                return { value: func(obj), done: false };
            } else {
                return { value: undefined, done: true };
            }
        },
        [Symbol.iterator]() { return this; }
    };
    return iter;
}


function jumpTo(timePosition) {
    const [seq, steps] = timePosition.toString().split(":", 2).map(s => parseInt(s, 16));

    host.createInstance("Debugger.Models.TTD.Position", seq, steps).SeekTo();
}

function timePos(timePosition) {
    const [seq, steps] = timePosition.toString().split(":", 2).map(s => parseInt(s, 16));

    return host.createInstance("Debugger.Models.TTD.Position", seq, steps);
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
