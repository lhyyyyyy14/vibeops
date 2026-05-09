# GB Internovel

GB Internovel 是一个 C++17 + SDL2 的掌机横屏互动小说 demo。它保留 `main.py`
作为早期 Python 原型，当前可运行版本在 `src/` 下实现：720x480 逻辑分辨率、
方向键/确认/取消交互、DeepSeek 生成剧情、SQLite 历史记录、关键词/风格开场设定。

## 当前功能

- 首页一级入口：故事开始、历史数据、设置。
- 开始故事前先选择关键词和风格：上下移动，Enter/A 确认，Esc/B 取消。
- 会话页按上下选择分支，Enter/A 确认，Esc/B 返回。
- DeepSeek API 生成故事段落和 A/B/X/Y 四个分支。
- SQLite 记录 session、开场设定、每轮故事、玩家选择、分支流程、prompt/raw response/debug 信息。
- 历史页只读查看历史 session、流程和单轮详情，高亮玩家选择。
- Windows 打包脚本会生成可直接转发给他人的开发预览目录。

## 构建和打包

Windows/MSYS2 环境下：

```powershell
powershell -ExecutionPolicy Bypass -File .\auto_package_windows.ps1 -MsysRoot D:\msys64
```

脚本会输出两个目录：

- `dist/APPS/GBInternovel/`：APPS 风格运行目录。
- `preview/GBInternovel-dev/`：可直接发给别人体验的 Windows 开发预览包。

发给别人时推荐压缩并发送整个 `preview/GBInternovel-dev/` 文件夹，不要只发 exe。
该目录会包含运行所需 DLL、`curl.exe`、证书、assets、空 key 的 `experiment_config.json`
和 `README.txt`。

## 配置和隐私

本地开发时复制：

```powershell
Copy-Item experiment_config.example.json experiment_config.json
```

然后在 `experiment_config.json` 写入 `api_key`，或设置环境变量 `DEEPSEEK_API_KEY`。
真实配置文件已被 `.gitignore` 忽略。

注意：`dist/APPS/GBInternovel/` 会复制本地 `experiment_config.json`，可能包含你的真实
API key；`preview/GBInternovel-dev/` 会改用 example 配置，默认空 key，更适合转发。

## 项目文档

更完整的项目结构、架构、数据表和运行流程见：

```text
docs/PROJECT_OVERVIEW.md
```
