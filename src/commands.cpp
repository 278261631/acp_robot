#include "commands.h"

#include "acp.h"
#include "config.h"
#include "filedialog.h"

#include <cstdio>

bool RunSelectScript(const std::wstring& target, std::wstring& result) {
    result.clear();
    if (target.empty()) {
        result = L"no script file specified (set script_file in config or pass a path)";
        return false;
    }
    if (!acp::ClickByLabel(g_cfg.btnSelect)) {
        result = L"failed to click the Select button";
        return false;
    }
    HWND dlg = filedialog::WaitFor(5000);
    if (!dlg) {
        result = L"file dialog did not appear within 5s";
        return false;
    }
    if (!filedialog::SetFileName(dlg, target)) {
        result = L"failed to set file name in dialog";
        return false;
    }
    filedialog::Accept(dlg);
    result = L"submitted script: " + target;
    return true;
}

bool ExecuteCommand(const std::wstring& verb, const std::wstring& arg, std::wstring& result) {
    result.clear();

    if (verb == L"select") {
        return RunSelectScript(arg.empty() ? g_cfg.scriptFile : arg, result);
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
    } else if (verb == L"select-script" || verb == L"open-script" || verb == L"selectscript") {
        return RunSelectScript(arg.empty() ? g_cfg.scriptFile : arg, result);
    } else if (verb == L"dialog-set") {
        if (arg.empty()) {
            result = L"usage: --dialog-set <full-file-path>";
            return false;
        }
        HWND dlg = filedialog::Find();
        if (!dlg) {
            result = L"no file dialog open";
            return false;
        }
        if (!filedialog::SetFileName(dlg, arg)) {
            result = L"failed to set file name";
            return false;
        }
        result = L"dialog-set: " + arg;
    } else if (verb == L"dialog-open") {
        HWND dlg = filedialog::Find();
        if (!dlg) {
            result = L"no file dialog open";
            return false;
        }
        filedialog::Accept(dlg);
        result = L"dialog-open: OK sent";
    } else if (verb == L"dialog-cancel") {
        HWND dlg = filedialog::Find();
        if (!dlg) {
            result = L"no file dialog open";
            return false;
        }
        filedialog::Cancel(dlg);
        result = L"dialog-cancel: cancel sent";
    } else if (verb == L"dialog-list") {
        HWND dlg = filedialog::Find();
        if (!dlg) {
            result = L"no file dialog open";
            return false;
        }
        result = filedialog::Describe(dlg);
    } else if (verb == L"help" || verb == L"-h") {
        result = L"usage: acp_robot.exe [--select|--run|--abort|--alert|--button <label>|--status|--list|--show-config|--config <path>]\r\n"
                 L"  --select [path]         click Select, fill the file dialog (default script_file), open it\r\n"
                 L"  --select-script [path]  alias of --select\r\n"
                 L"  --dialog-set <path>     set file name in the open dialog\r\n"
                 L"  --dialog-open           click Open in the open dialog\r\n"
                 L"  --dialog-cancel         click Cancel in the open dialog\r\n"
                 L"  --dialog-list           list controls of the open dialog";
    } else {
        result = L"unknown command: " + verb;
        return false;
    }

    return true;
}
