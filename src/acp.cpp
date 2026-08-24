#include "acp.h"
#include "config.h"

#include <tlhelp32.h>
#include <cwctype>
#include <cstring>
#include <cwchar>
#include <cstdio>

namespace {

bool iequals(const std::wstring& a, const std::wstring& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (std::towlower(a[i]) != std::towlower(b[i])) return false;
    return true;
}

std::wstring normalize(const std::wstring& s) {
    std::wstring r;
    r.reserve(s.size());
    bool space = false;
    for (wchar_t c : s) {
        if (std::iswspace(c)) { space = true; continue; }
        if (space && !r.empty()) r.push_back(L' ');
        space = false;
        r.push_back(c);
    }
    return r;
}

std::wstring canonical(const std::wstring& s) {
    std::wstring r = normalize(s);
    while (!r.empty()) {
        if (r.size() >= 3 && r.compare(r.size() - 3, 3, L"...") == 0) { r.erase(r.size() - 3); continue; }
        if (r.back() == L'\x2026') { r.pop_back(); continue; }
        break;
    }
    return r;
}

bool class_is(HWND h, const wchar_t* want) {
    wchar_t buf[64] = {};
    GetClassNameW(h, buf, 64);
    return wcscmp(buf, want) == 0;
}

std::wstring get_text(HWND h) {
    wchar_t buf[512] = {};
    int n = GetWindowTextW(h, buf, 512);
    return std::wstring(buf, n);
}

struct EnumCtx {
    DWORD pid;
    HWND form;
};

BOOL CALLBACK enum_top(HWND h, LPARAM l) {
    EnumCtx* ctx = reinterpret_cast<EnumCtx*>(l);
    DWORD pid = 0;
    GetWindowThreadProcessId(h, &pid);
    if (pid != ctx->pid) return TRUE;
    if (!class_is(h, g_cfg.formClass.c_str())) return TRUE;
    std::wstring title = get_text(h);
    if (g_cfg.formTitle.empty()) {
        if (IsWindowVisible(h)) { ctx->form = h; return FALSE; }
        if (!ctx->form) ctx->form = h;
        return TRUE;
    }
    if (iequals(title, g_cfg.formTitle)) { ctx->form = h; return FALSE; }
    if (title.find(g_cfg.formTitle) != std::wstring::npos && !ctx->form) ctx->form = h;
    return TRUE;
}

BOOL CALLBACK enum_btn(HWND h, LPARAM l) {
    auto* v = reinterpret_cast<std::vector<AcpButton>*>(l);
    if (class_is(h, g_cfg.buttonClass.c_str())) {
        AcpButton b;
        b.hwnd = h;
        b.label = get_text(h);
        b.visible = IsWindowVisible(h) != FALSE;
        b.enabled = IsWindowEnabled(h) != FALSE;
        v->push_back(b);
    }
    return TRUE;
}

} // namespace

namespace acp {

DWORD FindPid() {
    DWORD pid = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, g_cfg.processName.c_str()) == 0) { pid = pe.th32ProcessID; break; }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

bool IsRunning() {
    return FindPid() != 0;
}

HWND FindFormWindow() {
    DWORD pid = FindPid();
    if (!pid) return nullptr;
    EnumCtx ctx{ pid, nullptr };
    EnumWindows(enum_top, reinterpret_cast<LPARAM>(&ctx));
    return ctx.form;
}

bool EnumButtons(std::vector<AcpButton>& out) {
    out.clear();
    HWND form = FindFormWindow();
    if (!form) return false;
    EnumChildWindows(form, enum_btn, reinterpret_cast<LPARAM>(&out));
    return !out.empty();
}

bool FindButton(const std::wstring& label, AcpButton* out, bool fuzzy) {
    std::vector<AcpButton> btns;
    if (!EnumButtons(btns)) return false;
    for (auto& b : btns) {
        if (iequals(b.label, label)) { if (out) *out = b; return true; }
    }
    if (fuzzy) {
        std::wstring want = canonical(label);
        for (auto& b : btns) {
            if (iequals(canonical(b.label), want)) { if (out) *out = b; return true; }
        }
    }
    return false;
}

bool Click(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd) || !IsWindowEnabled(hwnd)) return false;
    RECT rc{};
    if (!GetWindowRect(hwnd, &rc)) return false;
    int cx = (rc.right - rc.left) / 2;
    int cy = (rc.bottom - rc.top) / 2;
    LPARAM lp = MAKELPARAM(cx, cy);
    SendMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lp);
    SendMessageW(hwnd, WM_LBUTTONUP, 0, lp);
    return true;
}

bool ClickByLabel(const std::wstring& label) {
    AcpButton b;
    if (!FindButton(label, &b)) return false;
    return Click(b.hwnd);
}

std::wstring StatusLine() {
    DWORD pid = FindPid();
    if (!pid) return L"ACP: not running";
    HWND form = FindFormWindow();
    std::wstring title = form ? get_text(form) : L"";
    wchar_t buf[256] = {};
    swprintf_s(buf, L"ACP: running (PID %lu)  window: %s", pid, title.empty() ? L"(none)" : title.c_str());
    return buf;
}

} // namespace acp
