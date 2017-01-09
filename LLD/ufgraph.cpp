
#include "DbgExts.h"
#include <shlwapi.h>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>

using namespace::std;


vector<string> split_by_char(const char* str, char c)
{
    if (str == nullptr || strlen(str) == 0) {
        throw exception("Address or name of the function to disassemble is required.");
    }

    string s(str);
    auto firstWordEnd = s.find(c, 0);
    if (firstWordEnd == string::npos) {
        return vector<string>(1, s);
    }
    auto secondWordBegin = s.find_first_not_of(c, firstWordEnd);
    if (secondWordBegin == string::npos) {
        return vector<string>(1, s);
    }
    return vector<string>({ s.substr(0, firstWordEnd), s.substr(secondWordBegin) });
}


string get_expected_ufgraph_path()
{
    char path[MAX_PATH];
    int pathLength = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (pathLength == MAX_PATH) {
        // check if the last error is ERROR_INSUFFICIENT_BUFFER
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            throw exception("The path is too long");
        }
    }
    string result(path, pathLength);
    auto indexOfSeparator = result.rfind('\\');
    if (indexOfSeparator == string::npos) {
        throw exception("Something is wrong with the windbg path.");
    }
    return result.erase(indexOfSeparator)
        .append("\\winext\\ufgraph.py");
}


void run_ufraph(IDebugControl* pDebugControl, const char* args)
{
    auto arguments = split_by_char(args, ' ');
    INT64 addr;
    if (arguments.size() > 1) {
        DEBUG_VALUE v = { 0 };
        if (FAILED(pDebugControl->Evaluate(arguments[1].c_str(), DEBUG_VALUE_INT64, &v, nullptr)))
        {
            throw exception("Could not resolve the current code address.");
        }
        addr = v.I64;
    }

    auto ufgraphPath = get_expected_ufgraph_path();

    if (!PathFileExistsA(ufgraphPath.c_str())) {
        throw exception("ufgraph.py could not be found. Download it from https://github.com/bfosterjr/ufgraph and place in the winext folder.");
    }
    
    stringstream command;
    command << ".shell -ci \"";
    if (arguments.size() > 1) {
        command << ".echo $ip=" << setfill('0') << setw(sizeof(int*) * 2) << std::hex << addr << ";";
    }
    command << "uf " << arguments[0] << "\"" << " python \"" << ufgraphPath << "\"";
    pDebugControl->Execute(DEBUG_OUTCTL_THIS_CLIENT, command.str().c_str(), DEBUG_EXECUTE_DEFAULT);
}

HRESULT CALLBACK ufgraph(IDebugClient *pDebugClient, PCSTR args)
{
    IDebugControl* pDebugControl;
    if (SUCCEEDED(pDebugClient->QueryInterface(__uuidof(IDebugControl), (void **)&pDebugControl))) {
        try {
            run_ufraph(pDebugControl, args);
        } catch (const exception &ex) {
            pDebugControl->Output(DEBUG_OUTPUT_ERROR, ex.what());
        }
        pDebugControl->Release();
        return S_OK;

    }
    return S_OK;
}
