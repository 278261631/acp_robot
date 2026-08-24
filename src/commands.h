#pragma once

#include <windows.h>
#include <string>

#define WM_ACPROBOT_CMD (WM_APP + 7)

bool ExecuteCommand(const std::wstring& verb, const std::wstring& arg, std::wstring& result);
bool RunSelectScript(const std::wstring& target, std::wstring& result);
bool RunRunFile(const std::wstring& target, std::wstring& result);
bool RunRunClick(std::wstring& result);
bool RunRunFill(std::wstring& result);
bool RunRunOpen(std::wstring& result);
bool RunAbort(std::wstring& result);
