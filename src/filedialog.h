#pragma once

#include <windows.h>
#include <string>

namespace filedialog {
HWND Find();
HWND WaitFor(int timeoutMs);
HWND FindFileNameEdit(HWND dlg);
bool SetFileName(HWND dlg, const std::wstring& path);
bool Accept(HWND dlg);
bool Cancel(HWND dlg);
std::wstring Describe(HWND dlg);
}
