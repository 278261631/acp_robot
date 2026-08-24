#include "config.h"

#include <windows.h>

#include <string>
#include <cwctype>
#include <cstdlib>

Config g_cfg;

static std::wstring Trim(const std::wstring& s) {
    size_t b = 0, e = s.size();
    while (b < e && std::iswspace(s[b])) ++b;
    while (e > b && std::iswspace(s[e - 1])) --e;
    return s.substr(b, e - b);
}

static std::wstring Lower(std::wstring s) {
    for (auto& c : s) c = static_cast<wchar_t>(std::towlower(c));
    return s;
}

static std::string ReadAllBytes(const std::wstring& path) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return std::string();
    std::string out;
    char buf[4096];
    DWORD rd = 0;
    while (ReadFile(h, buf, sizeof(buf), &rd, nullptr) && rd) out.append(buf, rd);
    CloseHandle(h);
    return out;
}

static bool WriteAllBytes(const std::wstring& path, const std::string& content) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD w = 0;
    bool ok = WriteFile(h, content.data(), static_cast<DWORD>(content.size()), &w, nullptr) != FALSE;
    CloseHandle(h);
    return ok;
}

static std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return L"";
    const char* data = s.data();
    int len = static_cast<int>(s.size());
    if (len >= 3 && static_cast<unsigned char>(data[0]) == 0xEF &&
        static_cast<unsigned char>(data[1]) == 0xBB &&
        static_cast<unsigned char>(data[2]) == 0xBF) {
        data += 3;
        len -= 3;
    }
    int n = MultiByteToWideChar(CP_UTF8, 0, data, len, nullptr, 0);
    if (n <= 0) return L"";
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, data, len, &w[0], n);
    return w;
}

static std::string WideToUtf8(const std::wstring& s) {
    if (s.empty()) return std::string();
    int n = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), nullptr, 0, nullptr, nullptr);
    if (n <= 0) return std::string();
    std::string out(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), &out[0], n, nullptr, nullptr);
    return out;
}

static void Apply(Config& cfg, const std::wstring& key, const std::wstring& val) {
    if (key == L"process_name") cfg.processName = val;
    else if (key == L"exe_path") cfg.exePath = val;
    else if (key == L"working_dir") cfg.workingDir = val;
    else if (key == L"form_class") cfg.formClass = val;
    else if (key == L"form_title") cfg.formTitle = val;
    else if (key == L"button_class") cfg.buttonClass = val;
    else if (key == L"select") cfg.btnSelect = val;
    else if (key == L"run") cfg.btnRun = val;
    else if (key == L"abort") cfg.btnAbort = val;
    else if (key == L"alert") cfg.btnAlert = val;
    else if (key == L"refresh_ms") {
        int v = _wtoi(val.c_str());
        if (v >= 100) cfg.refreshMs = v;
    }
}

static void ParseOneLine(Config& cfg, std::wstring line) {
    line = Trim(line);
    if (line.empty()) return;
    if (line[0] == L';' || line[0] == L'#' || line[0] == L'[') return;
    size_t eq = line.find(L'=');
    if (eq == std::wstring::npos) return;
    std::wstring k = Lower(Trim(line.substr(0, eq)));
    std::wstring v = Trim(line.substr(eq + 1));
    Apply(cfg, k, v);
}

static std::wstring DefaultConfigPath() {
    wchar_t buf[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring exe(buf);
    size_t pos = exe.find_last_of(L"\\/");
    if (pos == std::wstring::npos) return L"acp_robot.ini";
    return exe.substr(0, pos + 1) + L"acp_robot.ini";
}

bool LoadConfig(Config& cfg, const std::wstring& explicitPath) {
    cfg.configPath = explicitPath.empty() ? DefaultConfigPath() : explicitPath;

    std::string raw = ReadAllBytes(cfg.configPath);
    if (raw.empty()) {
        SaveConfig(cfg);
        return true;
    }

    std::wstring text = Utf8ToWide(raw);
    text += L'\n';
    std::wstring line;
    for (wchar_t c : text) {
        if (c == L'\n') {
            if (!line.empty() && line.back() == L'\r') line.pop_back();
            ParseOneLine(cfg, line);
            line.clear();
        } else {
            line.push_back(c);
        }
    }
    return true;
}

bool SaveConfig(const Config& cfg) {
    std::wstring out;
    out += L"; ACP Robot configuration\r\n";
    out += L"; Auto-generated with defaults. Edit to match your ACP installation.\r\n";
    out += L"\r\n";
    out += L"[acp]\r\n";
    out += L"process_name=" + cfg.processName + L"\r\n";
    out += L"exe_path=" + cfg.exePath + L"\r\n";
    out += L"working_dir=" + cfg.workingDir + L"\r\n";
    out += L"form_class=" + cfg.formClass + L"\r\n";
    out += L"form_title=" + cfg.formTitle + L"\r\n";
    out += L"button_class=" + cfg.buttonClass + L"\r\n";
    out += L"\r\n";
    out += L"[buttons]\r\n";
    out += L"select=" + cfg.btnSelect + L"\r\n";
    out += L"run=" + cfg.btnRun + L"\r\n";
    out += L"abort=" + cfg.btnAbort + L"\r\n";
    out += L"alert=" + cfg.btnAlert + L"\r\n";
    out += L"\r\n";
    out += L"[ui]\r\n";
    out += L"refresh_ms=" + std::to_wstring(cfg.refreshMs) + L"\r\n";

    std::string bytes = WideToUtf8(out);
    const char bom[3] = { static_cast<char>(0xEF), static_cast<char>(0xBB), static_cast<char>(0xBF) };
    std::string withBom;
    withBom.append(bom, 3);
    withBom += bytes;
    return WriteAllBytes(cfg.configPath, withBom);
}

const std::wstring& TrackedLabel(int i) {
    switch (i) {
    case 0: return g_cfg.btnSelect;
    case 1: return g_cfg.btnRun;
    case 2: return g_cfg.btnAbort;
    case 3: return g_cfg.btnAlert;
    }
    static const std::wstring empty;
    return empty;
}

int TrackedCount() {
    return 4;
}
