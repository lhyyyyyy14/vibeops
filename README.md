# GB Internovel

GB Internovel 是一个 C++17 + SDL2 的掌机横屏互动小说 demo。当前可运行版本在 `src/`
下实现，固定 720x480 逻辑分辨率，支持方向键、A/Enter 确认、B/Esc 返回。

## 当前功能

- 首页入口：开始故事、历史数据、设置中心。
- 开始故事前选择 1-5 个关键词和 1-3 个风格。
- 会话页显示 DeepSeek 生成的故事段落和 A/B/X/Y 四个分支。
- SQLite 记录 session、开场设定、每轮故事、玩家选择、分支流程和 debug 信息。
- 历史页只读查看历史 session、流程和单轮详情。
- Windows 打包脚本会生成可直接运行的预览目录。

## 构建和打包

Windows/MSYS2 环境下：

```powershell
powershell -ExecutionPolicy Bypass -File .\auto_package_windows.ps1
```

脚本会输出：

- `dist/APPS/GBInternovel/`：APPS 风格运行目录。
- `preview/GBInternovel-dev/`：可直接转发体验的 Windows 预览包。

转发时建议压缩整个 `preview/GBInternovel-dev/` 目录，不要只发送 exe。

## 配置和隐私

本地开发时复制：

```powershell
Copy-Item experiment_config.example.json experiment_config.json
```

然后在 `experiment_config.json` 写入 `api_key`，或设置环境变量 `DEEPSEEK_API_KEY`。
真实配置文件已被 `.gitignore` 忽略。

注意：`dist/APPS/GBInternovel/` 会复制本地 `experiment_config.json`，可能包含真实 API key；
`preview/GBInternovel-dev/` 会改用 example 配置，默认空 key，更适合转发。

## 项目文档

更完整的项目结构、架构、数据表和运行流程见：

```text
docs/PROJECT_OVERVIEW.md
```
