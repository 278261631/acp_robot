#include "window.h"

#include "acp.h"
#include "commands.h"
#include "config.h"

#include <shellapi.h>
#include <string>
#include <vector>
#include <cstring>
#include <cwchar>
#include <cstdio>

#define WM_TRAYICON (WM_APP + 1)

static const int IDC_STATUS = 1001;
static const int IDC_DUMP = 1002;
static const int IDC_REFRESH = 1003;
static const int IDC_OPENACP = 1004;
static const int IDC_BTN_BASE = 1100;
static const int IDC_STATE_BASE = 1200;
static const int IDC_CLICK_BASE = 1300;
static const int IDC_TRAY_SHOW = 4001;
static const int IDC_TRAY_OPENACP = 4002;
static const int IDC_TRAY_EXIT = 4003;

const wchar_t kWndClass[] = L"AcpRobotWindowClass";
const wchar_t kWndTitle[] = L"ACP Robot";

struct Ui {
    HINSTANCE hInst = nullptr;
    HWND hwnd = nullptr;
    HWND hStatus = nullptr;
    HWND hDump = nullptr;
    HWND hOpenAcp = nullptr;
    HWND hRefresh = nullptr;
    HWND hLabel[8] = {};
    HWND hState[8] = {};
    HWND hClick[8] = {};
    HFONT hFont = nullptr;
    NOTIFYICONDATAW nid{};
    bool trayAdded = false;
    bool firstHide = false;
};

static Ui g;

static void OpenAcp();
static void RefreshUi();
static void ExitApp();
static void ShowTrayMenu();
static void ShowBalloon(const std::wstring& title, const std::wstring& text);
static void AddTray();

static void RefreshUi() {
    SetWindowTextW(g.hStatus, acp::StatusLine().c_str());

    for (int i = 0; i < TrackedCount(); ++i) {
        AcpButton b;
        bool found = acp::FindButton(TrackedLabel(i), &b, true);
        const wchar_t* state = L"not found";
        bool clickable = false;
        if (found) {
            if (!b.visible) state = L"hidden";
            else if (!b.enabled) state = L"disabled";
            else { state = L"enabled"; clickable = true; }
        }
        SetWindowTextW(g.hState[i], state);
        EnableWindow(g.hClick[i], clickable ? TRUE : FALSE);
    }

    std::wstring dump;
    std::vector<AcpButton> btns;
    if (acp::EnumButtons(btns)) {
        for (auto& b : btns) {
            wchar_t line[256] = {};
            swprintf_s(line, L"[%s][%s] %s\r\n",
                b.enabled ? L"enabled" : L"disabled",
                b.visible ? L"vis" : L"hid",
                b.label.c_str());
            dump += line;
        }
    } else {
        dump = L"ACP not running or no command buttons found.\r\n";
    }
    SetWindowTextW(g.hDump, dump.c_str());
}

static void OpenAcp() {
    DWORD pid = acp::FindPid();
    if (pid) {
        HWND form = acp::FindFormWindow();
        if (form) {
            ShowWindow(form, SW_RESTORE);
            SetForegroundWindow(form);
            return;
        }
    }
    const std::wstring& path = g_cfg.exePath;
    const std::wstring& dir = g_cfg.workingDir;
    if (!path.empty() && GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES)
        ShellExecuteW(nullptr, L"open", path.c_str(), nullptr, dir.c_str(), SW_SHOWNORMAL);
}

static void ExitApp() {
    DestroyWindow(g.hwnd);
}

static void ShowFromTray() {
    ShowWindow(g.hwnd, SW_SHOW);
    SetForegroundWindow(g.hwnd);
}

static void ShowTrayMenu() {
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, IDC_TRAY_SHOW, L"Show ACP Robot");
    AppendMenuW(menu, MF_STRING, IDC_TRAY_OPENACP, L"Open ACP");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, IDC_TRAY_EXIT, L"Exit");

    POINT pt{};
    GetCursorPos(&pt);
    SetForegroundWindow(g.hwnd);
    int cmd = (int)TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
        pt.x, pt.y, 0, g.hwnd, nullptr);
    PostMessageW(g.hwnd, WM_NULL, 0, 0);
    DestroyMenu(menu);

    if (cmd == IDC_TRAY_SHOW) ShowFromTray();
    else if (cmd == IDC_TRAY_OPENACP) OpenAcp();
    else if (cmd == IDC_TRAY_EXIT) ExitApp();
}

static void ShowBalloon(const std::wstring& title, const std::wstring& text) {
    if (!g.trayAdded) return;
    NOTIFYICONDATAW n = g.nid;
    n.uFlags = NIF_INFO;
    n.szInfo[0] = 0;
    n.szInfoTitle[0] = 0;
    wcsncpy_s(n.szInfo, 256, text.c_str(), 255);
    wcsncpy_s(n.szInfoTitle, 64, title.c_str(), 63);
    n.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIconW(NIM_MODIFY, &n);
}

static void AddTray() {
    g.nid.cbSize = sizeof(g.nid);
    g.nid.hWnd = g.hwnd;
    g.nid.uID = 1;
    g.nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g.nid.uCallbackMessage = WM_TRAYICON;
    g.nid.hIcon = LoadIconW(nullptr, MAKEINTRESOURCEW(32512));
    wcscpy_s(g.nid.szTip, 128, L"ACP Robot");
    g.trayAdded = Shell_NotifyIconW(NIM_ADD, &g.nid) != FALSE;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        g.hwnd = hwnd;

        NONCLIENTMETRICSW ncm{};
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
        g.hFont = CreateFontIndirectW(&ncm.lfMessageFont);

        g.hStatus = CreateWindowExW(0, L"STATIC", L"",
            WS_CHILD | WS_VISIBLE, 10, 10, 540, 20,
            hwnd, (HMENU)(INT_PTR)IDC_STATUS, g.hInst, nullptr);

        for (int i = 0; i < TrackedCount(); ++i) {
            int y = 40 + i * 32;
            g.hLabel[i] = CreateWindowExW(0, L"STATIC", TrackedLabel(i).c_str(),
                WS_CHILD | WS_VISIBLE, 10, y, 175, 20,
                hwnd, (HMENU)(INT_PTR)(IDC_BTN_BASE + i), g.hInst, nullptr);
            g.hState[i] = CreateWindowExW(0, L"STATIC", L"...",
                WS_CHILD | WS_VISIBLE, 190, y, 120, 20,
                hwnd, (HMENU)(INT_PTR)(IDC_STATE_BASE + i), g.hInst, nullptr);
            g.hClick[i] = CreateWindowExW(0, L"BUTTON", L"Click",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 320, y - 2, 90, 24,
                hwnd, (HMENU)(INT_PTR)(IDC_CLICK_BASE + i), g.hInst, nullptr);
        }

        int dumpY = 40 + TrackedCount() * 32;
        g.hDump = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
            10, dumpY, 540, 250,
            hwnd, (HMENU)(INT_PTR)IDC_DUMP, g.hInst, nullptr);

        int btnY = dumpY + 250 + 8;
        g.hOpenAcp = CreateWindowExW(0, L"BUTTON", L"Open ACP",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 10, btnY, 90, 26,
            hwnd, (HMENU)(INT_PTR)IDC_OPENACP, g.hInst, nullptr);
        g.hRefresh = CreateWindowExW(0, L"BUTTON", L"Refresh",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 110, btnY, 90, 26,
            hwnd, (HMENU)(INT_PTR)IDC_REFRESH, g.hInst, nullptr);

        HWND all[] = { g.hStatus, g.hDump, g.hOpenAcp, g.hRefresh };
        for (HWND h : all) SendMessageW(h, WM_SETFONT, (WPARAM)g.hFont, TRUE);
        for (int i = 0; i < TrackedCount(); ++i) {
            SendMessageW(g.hLabel[i], WM_SETFONT, (WPARAM)g.hFont, TRUE);
            SendMessageW(g.hState[i], WM_SETFONT, (WPARAM)g.hFont, TRUE);
            SendMessageW(g.hClick[i], WM_SETFONT, (WPARAM)g.hFont, TRUE);
        }

        SetTimer(hwnd, 1, 1000, nullptr);
        RefreshUi();
        return 0;
    }

    case WM_TIMER:
        if (wp == 1) RefreshUi();
        return 0;

    case WM_COMMAND: {
        int id = LOWORD(wp);
        if (id == IDC_REFRESH) { RefreshUi(); return 0; }
        if (id == IDC_OPENACP) { OpenAcp(); RefreshUi(); return 0; }
        if (id >= IDC_CLICK_BASE && id < IDC_CLICK_BASE + TrackedCount()) {
            acp::ClickByLabel(TrackedLabel(id - IDC_CLICK_BASE));
            RefreshUi();
            return 0;
        }
        if (id == IDC_TRAY_SHOW) { ShowFromTray(); return 0; }
        if (id == IDC_TRAY_OPENACP) { OpenAcp(); return 0; }
        if (id == IDC_TRAY_EXIT) { ExitApp(); return 0; }
        return 0;
    }

    case WM_COPYDATA: {
        auto* cds = reinterpret_cast<COPYDATASTRUCT*>(lp);
        if (cds && cds->dwData == WM_ACPROBOT_CMD && cds->lpData && cds->cbData >= sizeof(wchar_t)) {
            std::wstring full(reinterpret_cast<wchar_t*>(cds->lpData), cds->cbData / sizeof(wchar_t));
            size_t sp = full.find(L' ');
            std::wstring verb = full.substr(0, sp);
            std::wstring arg = (sp == std::wstring::npos) ? L"" : full.substr(sp + 1);
            std::wstring result;
            bool ok = ExecuteCommand(verb, arg, result);
            RefreshUi();
            ShowBalloon(ok ? L"ACP Robot" : L"ACP Robot - failed", result);
        }
        return TRUE;
    }

    case WM_TRAYICON:
        switch (lp) {
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
            ShowFromTray();
            break;
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            ShowTrayMenu();
            break;
        }
        return 0;

    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        if (!g.firstHide) {
            g.firstHide = true;
            ShowBalloon(L"ACP Robot", L"Still running in the tray. Right-click the icon to exit.");
        }
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, 1);
        if (g.trayAdded) { Shell_NotifyIconW(NIM_DELETE, &g.nid); g.trayAdded = false; }
        if (g.hFont) { DeleteObject(g.hFont); g.hFont = nullptr; }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void RunUI(HINSTANCE hInstance, int nCmdShow) {
    g.hInst = hInstance;

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconW(nullptr, MAKEINTRESOURCEW(32512));
    wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = kWndClass;
    wc.hIconSm = LoadIconW(nullptr, MAKEINTRESOURCEW(32512));
    RegisterClassExW(&wc);

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT rc{ 0, 0, 560, 460 };
    AdjustWindowRectEx(&rc, style, FALSE, 0);
    g.hwnd = CreateWindowExW(0, kWndClass, kWndTitle, style,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInstance, nullptr);
    if (!g.hwnd) return;

    AddTray();
    ShowWindow(g.hwnd, nCmdShow);
    UpdateWindow(g.hwnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}
