#include <windows.h>
#include <shellapi.h>

#include <string>
#include <cwctype>

#include "acp.h"
#include "commands.h"
#include "config.h"
#include "window.h"

static const wchar_t* kMutexName = L"Local\\AcpRobot_SingleInstance";

struct CliOptions {
    std::wstring configPath;
    std::wstring verb;
    std::wstring arg;
    bool hasCmd = false;
};

static std::wstring Lower(std::wstring s) {
    for (auto& c : s) c = static_cast<wchar_t>(std::towlower(c));
    return s;
}

static CliOptions ParseArgs(int argc, wchar_t** argv) {
    CliOptions o;
    for (int i = 1; i < argc; ++i) {
        std::wstring t = argv[i];
        while (!t.empty() && (t[0] == L'-' || t[0] == L'/')) t.erase(t.begin());
        t = Lower(t);
        if (t == L"config") {
            if (i + 1 < argc) o.configPath = argv[++i];
        } else if (t == L"button") {
            o.hasCmd = true;
            o.verb = L"button";
            if (i + 1 < argc) o.arg = argv[++i];
        } else if (o.verb.empty()) {
            o.hasCmd = true;
            o.verb = t;
        } else {
            o.arg = argv[i];
        }
    }
    return o;
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

static void PrintConfig() {
    std::wstring s;
    s += L"config file: " + g_cfg.configPath + L"\r\n";
    s += L"process_name=" + g_cfg.processName + L"\r\n";
    s += L"exe_path=" + g_cfg.exePath + L"\r\n";
    s += L"working_dir=" + g_cfg.workingDir + L"\r\n";
    s += L"form_class=" + g_cfg.formClass + L"\r\n";
    s += L"form_title=" + g_cfg.formTitle + L"\r\n";
    s += L"button_class=" + g_cfg.buttonClass + L"\r\n";
    s += L"select=" + g_cfg.btnSelect + L"\r\n";
    s += L"run=" + g_cfg.btnRun + L"\r\n";
    s += L"abort=" + g_cfg.btnAbort + L"\r\n";
    s += L"alert=" + g_cfg.btnAlert + L"\r\n";
    s += L"refresh_ms=" + std::to_wstring(g_cfg.refreshMs) + L"\r\n";
    PrintLine(s);
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
    CliOptions o = ParseArgs(argc, argv);
    if (argv) LocalFree(argv);

    LoadConfig(g_cfg, o.configPath);

    if (o.hasCmd && o.verb == L"show-config") {
        PrintConfig();
        return 0;
    }

    HANDLE mutex = CreateMutexW(nullptr, TRUE, kMutexName);
    bool already = (GetLastError() == ERROR_ALREADY_EXISTS);

    if (already) {
        HWND wnd = FindWindowW(kWndClass, kWndTitle);
        if (o.hasCmd) {
            if (wnd) {
                ForwardCommand(wnd, o.verb, o.arg);
            } else {
                std::wstring result;
                ExecuteCommand(o.verb, o.arg, result);
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

    if (o.hasCmd) {
        std::wstring result;
        bool ok = ExecuteCommand(o.verb, o.arg, result);
        PrintLine(result);
        if (mutex) CloseHandle(mutex);
        return ok ? 0 : 1;
    }

    RunUI(hInstance, nCmdShow);
    if (mutex) CloseHandle(mutex);
    return 0;
}
