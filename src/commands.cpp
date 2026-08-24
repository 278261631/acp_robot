#include "commands.h"

#include "acp.h"
#include "config.h"
#include "filedialog.h"

#include <cstdio>

namespace {

std::wstring Hex(HWND h) {
    wchar_t buf[32] = {};
    swprintf_s(buf, L"0x%p", h);
    return buf;
}

std::wstring WinTitle(HWND h) {
    wchar_t buf[256] = {};
    GetWindowTextW(h, buf, 256);
    return buf;
}

} // namespace

static bool RunFileDialogFlow(const std::wstring& buttonLabel, const std::wstring& target, std::wstring& result) {
    result.clear();
    if (target.empty()) {
        result = L"no file specified (set it in config or pass a path)";
        return false;
    }
    if (!acp::ClickByLabel(buttonLabel)) {
        result = L"failed to click the button: '" + buttonLabel + L"'";
        return false;
    }
    HWND dlg = filedialog::WaitFor(5000);
    if (!dlg) {
        result = L"file dialog did not appear within 5s";
        return false;
    }
    if (!filedialog::OpenFileInDialog(dlg, target)) {
        result = L"failed to fill filename and click Open: '" + target + L"'";
        return false;
    }
    result = L"submitted: " + target;
    return true;
}

bool RunSelectScript(const std::wstring& target, std::wstring& result) {
    return RunFileDialogFlow(g_cfg.btnSelect, target, result);
}

bool RunRunFile(const std::wstring& target, std::wstring& result) {
    return RunFileDialogFlow(g_cfg.btnRun, target, result);
}

bool RunRunClick(std::wstring& result) {
    result.clear();

    HWND form = acp::FindFormWindow();
    if (!form) {
        result = L"step1 FAIL: ACP form not found. Lookup: process='" + g_cfg.processName +
                 L"', class='" + g_cfg.formClass + L"', title='" + g_cfg.formTitle + L"'";
        return false;
    }

    AcpButton b;
    if (!acp::FindButton(g_cfg.btnRun, &b)) {
        result = L"step1 FAIL: Run button not found in form " + Hex(form) +
                 L" ('" + WinTitle(form) + L"'). Lookup: class='" + g_cfg.buttonClass +
                 L"', label='" + g_cfg.btnRun + L"'";
        return false;
    }
    if (!b.visible || !b.enabled) {
        result = L"step1 FAIL: Run button " + Hex(b.hwnd) + L" not clickable (visible=" +
                 (b.visible ? L"yes" : L"no") + L", enabled=" + (b.enabled ? L"yes" : L"no") + L")";
        return false;
    }

    if (!acp::Click(b.hwnd)) {
        result = L"step1 FAIL: click message failed on Run button " + Hex(b.hwnd);
        return false;
    }

    HWND dlg = filedialog::WaitFor(5000);
    if (!dlg) {
        result = L"step1 FAIL: no dialog appeared within 5s after clicking Run button " +
                 Hex(b.hwnd) + L". Lookup: class='#32770' owned by form " + Hex(form);
        return false;
    }

    result = L"step1 OK: form " + Hex(form) + L" ('" + WinTitle(form) + L"') -> Run button " +
             Hex(b.hwnd) + L" ('" + b.label + L"') clicked -> dialog " + Hex(dlg) + L" appeared";
    return true;
}

bool RunRunFill(std::wstring& result) {
    result.clear();

    HWND dlg = filedialog::Find();
    if (!dlg) {
        result = L"step2 FAIL: no file dialog open (run step1 first). Lookup: class='#32770' owned by form";
        return false;
    }

    HWND field = filedialog::FindFileNameField(dlg);
    if (!field) {
        result = L"step2 FAIL: filename field not found in dialog " + Hex(dlg) +
                 L". Lookup: class='Edit'/'ComboBox', visible+enabled, id=1148\r\n" +
                 filedialog::Describe(dlg);
        return false;
    }

    std::wstring cls = filedialog::ClassName(field);

    if (!filedialog::SetFileName(dlg, g_cfg.runFile)) {
        result = L"step2 FAIL: could not set file name in field " + Hex(field) +
                 L" (class='" + cls + L"') of dialog " + Hex(dlg) +
                 L". run_file='" + g_cfg.runFile + L"'\r\n" + filedialog::Describe(dlg);
        return false;
    }

    result = L"step2 OK: dialog " + Hex(dlg) + L" -> filename field " + Hex(field) +
             L" (class='" + cls + L"', id=1148) set to run_file='" + g_cfg.runFile + L"'";
    return true;
}

bool RunRunOpen(std::wstring& result) {
    result.clear();

    HWND dlg = filedialog::Find();
    if (!dlg) {
        result = L"step3 FAIL: no file dialog open (run step1/step2 first). Lookup: class='#32770' owned by form";
        return false;
    }

    HWND open = filedialog::FindOpenButton(dlg);
    if (!open) {
        result = L"step3 FAIL: Open button not found in dialog " + Hex(dlg) +
                 L". Lookup: class='Button', id=1 (IDOK)";
        return false;
    }

    if (!PostMessageW(open, BM_CLICK, 0, 0)) {
        result = L"step3 FAIL: BM_CLICK failed on Open button " + Hex(open);
        return false;
    }

    for (int i = 0; i < 15; ++i) {
        if (!IsWindow(dlg)) break;
        Sleep(100);
    }

    result = L"step3 OK: Open button " + Hex(open) + L" (class='Button', id=IDOK) in dialog " +
             Hex(dlg) + L" clicked" + (IsWindow(dlg) ? L" (dialog still open)" : L" (dialog closed)");
    return true;
}

bool ExecuteCommand(const std::wstring& verb, const std::wstring& arg, std::wstring& result) {
    result.clear();

    if (verb == L"select") {
        return RunSelectScript(arg.empty() ? g_cfg.scriptFile : arg, result);
    } else if (verb == L"run") {
        return RunRunFile(arg.empty() ? g_cfg.runFile : arg, result);
    } else if (verb == L"run-click" || verb == L"run-1") {
        return RunRunClick(result);
    } else if (verb == L"run-fill" || verb == L"run-2") {
        return RunRunFill(result);
    } else if (verb == L"run-open" || verb == L"run-3") {
        return RunRunOpen(result);
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
                 L"  --run [path]            click Run, fill the file dialog (default run_file), open it (combined)\r\n"
                 L"  --run-click             step1: click Run, verify the file dialog appears\r\n"
                 L"  --run-fill              step2: find the filename edit, fill run_file\r\n"
                 L"  --run-open              step3: click Open in the file dialog\r\n"
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
