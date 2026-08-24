#include "commands.h"

#include "acp.h"
#include "config.h"

#include <cstdio>

bool ExecuteCommand(const std::wstring& verb, const std::wstring& arg, std::wstring& result) {
    result.clear();

    if (verb == L"select") {
        if (!acp::ClickByLabel(g_cfg.btnSelect)) {
            result = L"button not found or disabled: '" + g_cfg.btnSelect + L"'";
            return false;
        }
        result = L"clicked: " + g_cfg.btnSelect;
    } else if (verb == L"run") {
        if (!acp::ClickByLabel(g_cfg.btnRun)) {
            result = L"button not found or disabled: '" + g_cfg.btnRun + L"'";
            return false;
        }
        result = L"clicked: " + g_cfg.btnRun;
    } else if (verb == L"abort") {
        if (!acp::ClickByLabel(g_cfg.btnAbort)) {
            result = L"button not found or disabled: '" + g_cfg.btnAbort + L"'";
            return false;
        }
        result = L"clicked: " + g_cfg.btnAbort;
    } else if (verb == L"alert") {
        if (!acp::ClickByLabel(g_cfg.btnAlert)) {
            result = L"button not found or disabled: '" + g_cfg.btnAlert + L"'";
            return false;
        }
        result = L"clicked: " + g_cfg.btnAlert;
    } else if (verb == L"button") {
        if (arg.empty()) {
            result = L"--button requires a label argument";
            return false;
        }
        if (!acp::ClickByLabel(arg)) {
            result = L"button not found or disabled: '" + arg + L"'";
            return false;
        }
        result = L"clicked: " + arg;
    } else if (verb == L"status") {
        result = acp::StatusLine();
        std::vector<AcpButton> btns;
        if (acp::EnumButtons(btns)) {
            result += L"\r\n";
            for (auto& b : btns) {
                wchar_t line[256] = {};
                swprintf_s(line, L"  [%s] %s", b.enabled ? L"enabled" : L"disabled", b.label.c_str());
                result += line;
                result += L"\r\n";
            }
        } else {
            result += L"\r\n  (no command buttons found)";
        }
    } else if (verb == L"list") {
        std::vector<AcpButton> btns;
        if (!acp::EnumButtons(btns)) {
            result = L"ACP not running or no command buttons found.";
            return false;
        }
        for (auto& b : btns) {
            wchar_t line[256] = {};
            swprintf_s(line, L"[%s][%s] 0x%p  %s",
                b.enabled ? L"enabled" : L"disabled",
                b.visible ? L"vis" : L"hid",
                b.hwnd, b.label.c_str());
            result += line;
            result += L"\r\n";
        }
    } else if (verb == L"help" || verb == L"-h") {
        result = L"usage: acp_robot.exe [--select|--run|--abort|--alert|--button <label>|--status|--list|--show-config|--config <path>]";
    } else {
        result = L"unknown command: " + verb;
        return false;
    }

    return true;
}
