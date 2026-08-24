#pragma once

#include <windows.h>
#include <string>
#include <vector>

struct AcpButton {
    HWND hwnd = nullptr;
    std::wstring label;
    bool visible = false;
    bool enabled = false;
};

namespace acp {
DWORD FindPid();
bool IsRunning();
HWND FindFormWindow();
bool EnumButtons(std::vector<AcpButton>& out);
bool FindButton(const std::wstring& label, AcpButton* out, bool fuzzy = false);
bool Click(HWND hwnd);
bool ClickByLabel(const std::wstring& label);
std::wstring StatusLine();
}
