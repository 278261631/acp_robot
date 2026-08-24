#include "filedialog.h"

#include "acp.h"

#include <string>
#include <cstring>
#include <cwchar>
#include <cstdio>

namespace {

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

struct DlgCtx {
    HWND owner;
    HWND found;
};

BOOL CALLBACK enum_dlg(HWND h, LPARAM l) {
    DlgCtx* c = reinterpret_cast<DlgCtx*>(l);
    if (!class_is(h, L"#32770")) return TRUE;
    if (!IsWindowVisible(h)) return TRUE;
    if (c->owner && GetWindow(h, GW_OWNER) == c->owner) { c->found = h; return FALSE; }
    if (!c->found) c->found = h;
    return TRUE;
}

struct EditCtx {
    HWND exact;
    HWND fallback;
};

BOOL CALLBACK enum_edit(HWND h, LPARAM l) {
    EditCtx* c = reinterpret_cast<EditCtx*>(l);
    if (!class_is(h, L"Edit")) return TRUE;
    if (!IsWindowVisible(h) || !IsWindowEnabled(h)) return TRUE;
    if (GetDlgCtrlID(h) == 1148) { c->exact = h; return FALSE; }
    if (!c->fallback) c->fallback = h;
    return TRUE;
}

} // namespace

namespace filedialog {

HWND Find() {
    HWND owner = acp::FindFormWindow();
    DlgCtx ctx{ owner, nullptr };
    EnumWindows(enum_dlg, reinterpret_cast<LPARAM>(&ctx));
    return ctx.found;
}

HWND WaitFor(int timeoutMs) {
    int step = 100;
    int waited = 0;
    while (waited < timeoutMs) {
        HWND d = Find();
        if (d) return d;
        Sleep(step);
        waited += step;
    }
    return nullptr;
}

HWND FindFileNameEdit(HWND dlg) {
    if (!dlg) return nullptr;
    EditCtx ctx{ nullptr, nullptr };
    EnumChildWindows(dlg, enum_edit, reinterpret_cast<LPARAM>(&ctx));
    return ctx.exact ? ctx.exact : ctx.fallback;
}

bool SetFileName(HWND dlg, const std::wstring& path) {
    HWND edit = FindFileNameEdit(dlg);
    if (!edit) return false;
    return SendMessageW(edit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(path.c_str())) != FALSE;
}

bool Accept(HWND dlg) {
    if (!dlg) return false;
    return PostMessageW(dlg, WM_COMMAND, IDOK, 0) != FALSE;
}

bool Cancel(HWND dlg) {
    if (!dlg) return false;
    return PostMessageW(dlg, WM_COMMAND, IDCANCEL, 0) != FALSE;
}

std::wstring Describe(HWND dlg) {
    std::wstring s;
    if (!dlg) { s = L"(no file dialog)"; return s; }
    wchar_t hdr[160] = {};
    swprintf_s(hdr, L"dialog 0x%p: '%s'\r\n", dlg, get_text(dlg).c_str());
    s += hdr;

    EnumChildWindows(dlg, [](HWND h, LPARAM l) -> BOOL {
        auto* out = reinterpret_cast<std::wstring*>(l);
        wchar_t cls[64] = {};
        GetClassNameW(h, cls, 64);
        std::wstring txt = get_text(h);
        wchar_t line[320] = {};
        swprintf_s(line, L"  id=%d  [%s]  %s\r\n", GetDlgCtrlID(h), cls, txt.c_str());
        *out += line;
        return TRUE;
    }, reinterpret_cast<LPARAM>(&s));

    return s;
}

} // namespace filedialog
