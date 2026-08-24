# ACP Robot

Windows 桌面工具（C++17 / Win32 API，CMake 构建），用于自动化控制 **ACP Observatory Control Software**（天文台控制软件）：定位主窗体、点击按钮、自动填写文件选择对话框、监控并确认异常弹窗。

## 构建

```bat
cmake -S . -B build
cmake --build build --config Release
```

生成 `build\Release\acp_robot.exe`。首次运行会在 exe 同目录生成 `acp_robot.ini` 配置（UTF-8 带 BOM）。

## 配置文件 acp_robot.ini

| 键 | 默认值 | 说明 |
| --- | --- | --- |
| `process_name` | `acp.exe` | ACP 进程名（用于定位） |
| `exe_path` | `C:\Program Files (x86)\ACP Obs Control\acp.exe` | 打开 ACP 用的路径 |
| `working_dir` | `C:\Program Files (x86)\ACP Obs Control` | 工作目录 |
| `form_class` | `ThunderRT6FormDC` | ACP 主窗体类名 |
| `form_title` | `ACP Observatory Control Software` | 主窗体标题 |
| `button_class` | `ThunderRT6CommandButton` | 命令按钮类名 |
| `select` / `run` / `abort` / `alert` | 按钮文案 | 跟踪的按钮 |
| `script_file` | `AcquireImages.js` | select 默认脚本 |
| `run_file` | `C:\Users\Administrator\Documents\ACP Astronomy\Plans\1.txt` | run 默认计划文件 |
| `refresh_ms` | `1000` | UI 刷新间隔（毫秒） |
| `abort_timeout_ms` | `30000` | abort 等待按钮禁用超时（毫秒） |

## 命令行用法

> 所有命令默认读取 exe 同目录的 `acp_robot.ini`；可用 `--config <path>` 指定其他配置文件。
> 若程序已在运行，命令会被转发给 UI 实例执行（结果通过托盘通知/日志体现）；否则在本进程执行并打印到控制台。

### 查看状态与信息

```bat
acp_robot.exe --status
acp_robot.exe --list
acp_robot.exe --show-config
acp_robot.exe --config C:\my\acp_robot.ini --show-config
```

### 选择脚本（自动填文件对话框）

```bat
:: 使用配置里的 script_file
acp_robot.exe --select

:: 指定其他脚本
acp_robot.exe --select D:\plans\AcquireImages.js
acp_robot.exe --select-script D:\plans\AcquireImages.js
```

### 运行计划（整体流程）

```bat
:: 使用配置里的 run_file：点击 Run → 填文件名 → 点打开
acp_robot.exe --run
acp_robot.exe --run C:\Users\Administrator\Documents\ACP Astronomy\Plans\2.txt
```

### 运行计划（分步执行，便于排查）

```bat
:: 1) 点击 Run 按钮，并验证文件对话框弹出
acp_robot.exe --run-click

:: 2) 在弹框中找到文件名输入框，填入 run_file
acp_robot.exe --run-fill

:: 3) 点击弹框的“打开”按钮
acp_robot.exe --run-open
```

### 中止（Abort，带结束判定）

```bat
:: 点击 Abort，等待按钮变为禁用（默认最多 30s，可在 ini 里改 abort_timeout_ms）
:: 超时则返回非 0 退出码，并打印失败原因
acp_robot.exe --abort
```

### 其他按钮

```bat
acp_robot.exe --alert
acp_robot.exe --button "Shutdown"
```

### 文件对话框手工操作

```bat
acp_robot.exe --dialog-list
acp_robot.exe --dialog-set C:\temp\plan.txt
acp_robot.exe --dialog-open
acp_robot.exe --dialog-cancel
```

## UI 说明

- 底部按钮：`Open ACP`（打开/前置 ACP）、`Refresh`（刷新）、`1.Run+Verify`（Run 步骤 1）、`2.Fill File`（Run 步骤 2）、`3.Open Dlg`（Run 步骤 3）、`Clear Log`（清空日志）
- 日志区实时显示各关键组件句柄与查找依据（如 `form 0x... -> Run button 0x... clicked -> dialog 0x...`）
- 内置监控：每 2 秒检测标题或内容含 "abort" 的弹窗（如 "Aborting failed."），自动点击"确定"，每次点击计数并记录到日志（`abort monitor: clicked OK #N ...`）
- 关闭窗口最小化到托盘，退出需右键托盘图标 → Exit
