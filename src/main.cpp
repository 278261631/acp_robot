#include <windows.h>
#include <shellapi.h>

#include <string>
#include <cwctype>

#include "acp.h"
#include "commands.h"
#include "window.h"

static const wchar_t* kMutexName = L"Local\\AcpRobot_SingleInstance";

static std::wstring Lower(std::wstring s) {
    for (auto& c : s) c = static_cast<wchar_t>(std::towlower(c));
    return s;
}

static bool ParseArgs(int argc, wchar_t** argv, std::wstring& verb, std::wstring& arg) {
    if (argc < 2) return false;
    std::wstring a = argv[1];
    while (!a.empty() && (a[0] == L'-' || a[0] == L'/')) a.erase(a.begin());
    verb = Lower(a);
    arg.clear();
    if (verb == L"button" && argc >= 3) arg = argv[2];
    return true;
}

static void PrintLine(const std::wstring& s) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    bool opened = false;
    if (!h || h == INVALID_HANDLE_VALUE) {
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            h = CreateFileW(L"CONOUT$", GENERIC_WRITE, FILE_SHARE_WRITE,
                nullptr, OPEN_EXISTING, 0, nullptr);
            opened = true;
        }
    }
    if (!h || h == INVALID_HANDLE_VALUE) return;

    std::string bytes;
    bytes.reserve(s.size() + 2);
    for (wchar_t c : s) {
        if (c == L'\r' || c == L'\n') bytes.push_back(static_cast<char>(c));
        else if (c >= 0x20 && c <= 0x7E) bytes.push_back(static_cast<char>(c));
        else bytes.push_back('?');
    }
    bytes += "\r\n";

    DWORD w;
    WriteFile(h, bytes.data(), static_cast<DWORD>(bytes.size()), &w, nullptr);
    if (opened) CloseHandle(h);
}

static bool ForwardCommand(HWND target, const std::wstring& verb, const std::wstring& arg) {
    std::wstring full = verb;
    if (!arg.empty()) { full += L' '; full += arg; }
    COPYDATASTRUCT cds{};
    cds.dwData = WM_ACPROBOT_CMD;
    cds.cbData = static_cast<DWORD>((full.size() + 1) * sizeof(wchar_t));
    cds.lpData = const_cast<wchar_t*>(full.c_str());
    return SendMessageW(target, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cds)) != 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    SetProcessDPIAware();

    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::wstring verb, arg;
    bool hasCmd = ParseArgs(argc, argv, verb, arg);
    if (argv) LocalFree(argv);

    HANDLE mutex = CreateMutexW(nullptr, TRUE, kMutexName);
    bool already = (GetLastError() == ERROR_ALREADY_EXISTS);

    if (already) {
        HWND wnd = FindWindowW(kWndClass, kWndTitle);
        if (hasCmd) {
            if (wnd) {
                ForwardCommand(wnd, verb, arg);
            } else {
                std::wstring result;
                ExecuteCommand(verb, arg, result);
                PrintLine(result);
            }
        } else {
            if (wnd) {
                ShowWindow(wnd, SW_RESTORE);
                SetForegroundWindow(wnd);
            }
        }
        if (mutex) CloseHandle(mutex);
        return 0;
    }

    if (hasCmd) {
        std::wstring result;
        bool ok = ExecuteCommand(verb, arg, result);
        PrintLine(result);
        if (mutex) CloseHandle(mutex);
        return ok ? 0 : 1;
    }

    RunUI(hInstance, nCmdShow);
    if (mutex) CloseHandle(mutex);
    return 0;
}
