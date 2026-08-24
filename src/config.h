#pragma once

#include <string>

struct Config {
    std::wstring processName = L"acp.exe";
    std::wstring exePath = L"C:\\Program Files (x86)\\ACP Obs Control\\acp.exe";
    std::wstring workingDir = L"C:\\Program Files (x86)\\ACP Obs Control";
    std::wstring formClass = L"ThunderRT6FormDC";
    std::wstring formTitle = L"ACP Observatory Control Software";
    std::wstring buttonClass = L"ThunderRT6CommandButton";

    std::wstring btnSelect = L"Select the Script ...";
    std::wstring btnRun = L"Run";
    std::wstring btnAbort = L"Abort";
    std::wstring btnAlert = L"Alert";

    std::wstring scriptFile = L"AcquireImages.js";
    std::wstring runFile = L"C:\\Users\\Administrator\\Documents\\ACP Astronomy\\Plans\\1.txt";

    int refreshMs = 1000;

    std::wstring configPath;
};

extern Config g_cfg;

bool LoadConfig(Config& cfg, const std::wstring& explicitPath = L"");
bool SaveConfig(const Config& cfg);

const std::wstring& TrackedLabel(int i);
int TrackedCount();
