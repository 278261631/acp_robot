#pragma once

#include <windows.h>
#include <string>

namespace filedialog {
HWND Find();
HWND WaitFor(int timeoutMs);
HWND FindFileNameEdit(HWND dlg);
HWND WaitForFileNameEdit(HWND dlg, int timeoutMs);
HWND FindFileNameField(HWND dlg);
HWND WaitForFileNameField(HWND dlg, int timeoutMs);
std::wstring ClassName(HWND h);
bool SetFileName(HWND dlg, const std::wstring& path);
HWND FindOpenButton(HWND dlg);
bool Accept(HWND dlg);
bool Cancel(HWND dlg);
bool OpenFileInDialog(HWND dlg, const std::wstring& path);
std::wstring Describe(HWND dlg);
}
