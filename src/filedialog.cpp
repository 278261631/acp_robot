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

struct FindCtx {
    const wchar_t* cls;
    int id;
    HWND result;
};

BOOL CALLBACK enum_find(HWND h, LPARAM l) {
    FindCtx* c = reinterpret_cast<FindCtx*>(l);
    if (class_is(h, c->cls) && GetDlgCtrlID(h) == c->id) { c->result = h; return FALSE; }
    return TRUE;
}

HWND find_ctrl(HWND parent, const wchar_t* cls, int id) {
    FindCtx ctx{ cls, id, nullptr };
    EnumChildWindows(parent, enum_find, reinterpret_cast<LPARAM>(&ctx));
    return ctx.result;
}

struct DlgCtx {
    DWORD pid;
    HWND owner;
    HWND found;
};

BOOL CALLBACK enum_dlg(HWND h, LPARAM l) {
    DlgCtx* c = reinterpret_cast<DlgCtx*>(l);
    if (!class_is(h, L"#32770")) return TRUE;
    if (!IsWindowVisible(h)) return TRUE;
    DWORD pid = 0;
    GetWindowThreadProcessId(h, &pid);
    if (pid != c->pid) return TRUE;
    if (c->owner && GetWindow(h, GW_OWNER) == c->owner) { c->found = h; return FALSE; }
    if (!c->found) c->found = h;
    return TRUE;
}

BOOL CALLBACK enum_edit(HWND h, LPARAM l) {
    HWND* result = reinterpret_cast<HWND*>(l);
    if (!class_is(h, L"Edit")) return TRUE;
    if (!IsWindowVisible(h) || !IsWindowEnabled(h)) return TRUE;
    if (GetDlgCtrlID(h) == 1148) { *result = h; return FALSE; }
    return TRUE;
}

struct FieldCtx {
    HWND edit = nullptr;
    HWND combo = nullptr;
};

BOOL CALLBACK enum_field(HWND h, LPARAM l) {
    FieldCtx* c = reinterpret_cast<FieldCtx*>(l);
    wchar_t cls[64] = {};
    GetClassNameW(h, cls, 64);
    if (!IsWindowVisible(h) || !IsWindowEnabled(h)) return TRUE;
    if (GetDlgCtrlID(h) != 1148) return TRUE;
    if (wcscmp(cls, L"Edit") == 0) { c->edit = h; return TRUE; }
    if (wcscmp(cls, L"ComboBox") == 0 || wcscmp(cls, L"ComboBoxEx32") == 0) {
        if (!c->combo) c->combo = h;
        return TRUE;
    }
    return TRUE;
}

std::wstring field_text(HWND field) {
    wchar_t buf[1024] = {};
    GetWindowTextW(field, buf, 1024);
    return buf;
}

HWND find_descendant_edit(HWND parent) {
    HWND out = nullptr;
    EnumChildWindows(parent, [](HWND h, LPARAM l) -> BOOL {
        HWND* r = reinterpret_cast<HWND*>(l);
        wchar_t cls[64] = {};
        GetClassNameW(h, cls, 64);
        if (wcscmp(cls, L"Edit") == 0) { *r = h; return FALSE; }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&out));
    return out;
}

} // namespace

namespace filedialog {

std::wstring ClassName(HWND h) {
    wchar_t buf[64] = {};
    GetClassNameW(h, buf, 64);
    return buf;
}

HWND Find() {
    DlgCtx ctx{ acp::FindPid(), acp::FindFormWindow(), nullptr };
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
    HWND result = nullptr;
    EnumChildWindows(dlg, enum_edit, reinterpret_cast<LPARAM>(&result));
    return result;
}

HWND WaitForFileNameEdit(HWND dlg, int timeoutMs) {
    int step = 50;
    int waited = 0;
    while (waited < timeoutMs) {
        HWND e = FindFileNameEdit(dlg);
        if (e) return e;
        Sleep(step);
        waited += step;
    }
    return nullptr;
}

HWND FindFileNameField(HWND dlg) {
    if (!dlg) return nullptr;
    FieldCtx ctx;
    EnumChildWindows(dlg, enum_field, reinterpret_cast<LPARAM>(&ctx));
    if (ctx.combo) return ctx.combo;
    return ctx.edit;
}

HWND WaitForFileNameField(HWND dlg, int timeoutMs) {
    int step = 50;
    int waited = 0;
    while (waited < timeoutMs) {
        HWND f = FindFileNameField(dlg);
        if (f) return f;
        Sleep(step);
        waited += step;
    }
    return nullptr;
}

bool SetFileName(HWND dlg, const std::wstring& path) {
    HWND field = WaitForFileNameField(dlg, 3000);
    if (!field) return false;

    HWND edit = find_descendant_edit(field);
    if (!edit) edit = field;

    bool ok = false;
    for (int attempt = 0; attempt < 5 && !ok; ++attempt) {
        if (field != edit) {
            SendMessageW(field, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
            SendMessageW(field, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(path.c_str()));
        }
        SendMessageW(edit, EM_SETSEL, 0, -1);
        SendMessageW(edit, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(path.c_str()));
        Sleep(100);
        if (field_text(edit) == path) { ok = true; break; }
        SetWindowTextW(edit, path.c_str());
        SetWindowTextW(field, path.c_str());
        Sleep(100);
        if (field_text(edit) == path) { ok = true; break; }
    }
    if (!ok) return false;

    for (int i = 0; i < 5 && ok; ++i) {
        Sleep(200);
        if (field_text(edit) != path) {
            SendMessageW(edit, EM_SETSEL, 0, -1);
            SendMessageW(edit, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(path.c_str()));
            if (field_text(edit) != path) { ok = false; break; }
        }
    }
    return ok;
}

HWND FindOpenButton(HWND dlg) {
    if (!dlg) return nullptr;
    return find_ctrl(dlg, L"Button", IDOK);
}

bool Accept(HWND dlg) {
    if (!dlg) return false;
    return PostMessageW(dlg, WM_COMMAND, IDOK, 0) != FALSE;
}

bool Cancel(HWND dlg) {
    if (!dlg) return false;
    return PostMessageW(dlg, WM_COMMAND, IDCANCEL, 0) != FALSE;
}

// Fills the file name and presses Open using window messages only.
// No foreground activation, no real mouse/keyboard input: the dialog may be
// covered by other windows while this runs.
bool OpenFileInDialog(HWND dlg, const std::wstring& path) {
    if (!dlg) return false;

    if (!SetFileName(dlg, path)) return false;

    // Press Open: BM_CLICK on the button makes it post a proper WM_COMMAND
    // to the dialog; fall back to posting WM_COMMAND directly.
    HWND open = find_ctrl(dlg, L"Button", IDOK);
    if (open) PostMessageW(open, BM_CLICK, 0, 0);
    else PostMessageW(dlg, WM_COMMAND, IDOK, 0);

    for (int i = 0; i < 15; ++i) {
        if (!IsWindow(dlg)) return true;
        Sleep(100);
    }

    PostMessageW(dlg, WM_COMMAND, IDOK, 0);
    for (int i = 0; i < 15; ++i) {
        if (!IsWindow(dlg)) return true;
        Sleep(100);
    }
    return false;
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
