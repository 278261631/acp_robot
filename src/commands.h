#pragma once

#include <windows.h>
#include <string>

#define WM_ACPROBOT_CMD (WM_APP + 7)

bool ExecuteCommand(const std::wstring& verb, const std::wstring& arg, std::wstring& result);
