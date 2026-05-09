# GB Internovel Project Overview

## 项目定位

GB Internovel 是一个面向复古 Linux 掌机和 Windows 预览的互动小说 demo。当前主线是
`C++17 + SDL2` 原生应用，目标逻辑分辨率固定为 `720x480`，交互方式收敛为：

- 方向键：移动光标或切换项目。
- Enter / A：确认。
- Esc / B：取消或返回。

`main.py` 是 Python 原型，保留用于提示词和实验逻辑参考；实际运行版本以 `src/` 为准。

## 当前实现功能

- **故事设定**：开始前选择 1-5 个关键词和 1-3 个风格，再确认生成开场。
- **AI 会话**：调用 DeepSeek Chat Completions API，生成一段故事和 A/B/X/Y 四个分支。
- **分支交互**：会话页用上下选择分支，Enter/A 确认，后续剧情延续当前 session。
- **历史存储**：SQLite 记录会话、开场设定、故事轮次、玩家选择、分支边、prompt/raw response/debug 信息。
- **历史查看**：只读浏览历史 session、流程和单轮详情，高亮当时玩家选择。
- **Windows 预览包**：`auto_package_windows.ps1` 同时生成掌机 APPS 目录和可直接转发的开发预览目录。

## 目录结构

```text
.
├─ src/
│  ├─ main.cpp                  # SDL 初始化、主循环、场景分发
│  ├─ app_config.*              # native_config.ini 运行时配置
│  ├─ input_manager.*           # 键盘/手柄输入映射
│  ├─ layout_metrics.*          # 720x480 布局常量
│  ├─ scene_manager.*           # 当前场景和跨场景 StorySetup 暂存
│  ├─ ui_draw.*                 # SDL 绘制和文本渲染
│  ├─ scenes/
│  │  ├─ boot_scene.*           # 启动画面
│  │  ├─ home_scene.*           # 首页入口
│  │  ├─ story_setup_scene.*    # 关键词/风格选择流程
│  │  ├─ session_scene.*        # AI 故事会话
│  │  ├─ history_scene.*        # 历史浏览
│  │  └─ settings_scene.*       # 设置说明页
│  └─ business/
│     ├─ ai_client.*            # DeepSeek 请求、prompt 构造、响应解析
│     ├─ story_session.*        # 会话状态机和历史写入
│     ├─ story_setup.*          # 内置关键词/风格和摘要工具
│     ├─ history_store.*        # SQLite 封装
│     └─ choice.h               # 分支选项结构
├─ assets/                      # 字体等资源
├─ docs/                        # 项目文档
├─ dist/                        # 打包输出，忽略
├─ preview/                     # 可转发开发预览包，忽略
├─ data/                        # 本地 SQLite 历史数据，忽略
├─ experiment_config.example.json
├─ native_config.ini
├─ native_keymap.ini
├─ Makefile
└─ auto_package_windows.ps1
```

## 整体架构

应用分为三层：

- **Runtime/Core**：`main.cpp`、`InputManager`、`SceneManager`、`LayoutMetrics`、`ui_draw`。
  这一层负责 SDL 生命周期、输入抽象、场景切换和基础 UI 绘制。
- **Scenes/UI**：`Boot/Home/StorySetup/Session/History/Settings`。
  每个场景只处理本页面的输入、状态展示和跳转，不直接调用 SQL。
- **Business**：`AiClient`、`StorySession`、`StorySetup`、`HistoryStore`。
  这一层负责 prompt、API、会话状态、SQLite 持久化和业务数据结构。

关键数据流：

```text
Home
  -> StorySetupScene
  -> SceneManager.pending_story_setup
  -> SessionScene
  -> StorySession.StartNew(setup)
  -> AiClient.GenerateStoryTurn()
  -> HistoryStore 写 sessions/story_setups/turns/turn_edges
```

历史查看反向读取：

```text
HistoryScene
  -> HistoryStore.ListSessions()
  -> HistoryStore.LoadTurns()
  -> HistoryStore.LoadEdges()
```

## SQLite 数据表

数据库默认位置：

```text
data/gb_internovel.db
```

可通过环境变量覆盖目录：

```text
GBINTERNOVEL_DATA_DIR
```

当前表：

- `sessions`：一局故事会话，记录模型、世界观、创建/更新时间。
- `story_setups`：本局开场设定，记录关键词、风格和第一轮 turn id。
- `turns`：每次 AI 展示的故事节点，记录故事、四个选项、玩家选择、prompt/raw response/error/latency。
- `turn_edges`：真实游玩流程边，记录从哪一轮通过哪个选择进入下一轮。

数据库由 `HistoryStore::InitSchema()` 自动创建和迁移缺失表，不需要手动执行 SQL。

## 打包输出

Windows 打包命令：

```powershell
powershell -ExecutionPolicy Bypass -File .\auto_package_windows.ps1 -MsysRoot D:\msys64
```

输出：

- `dist/APPS/GBInternovel/`：完整运行目录，用于 APPS 风格部署。
- `preview/GBInternovel-dev/`：可直接压缩转发的 Windows 开发预览目录。

`preview/GBInternovel-dev/` 会排除本机 `data/` 和 runtime log，并用
`experiment_config.example.json` 覆盖生成空 key 的 `experiment_config.json`，降低误泄漏 API key 的风险。

## 依赖

- C++17 编译器。
- SDL2。
- SDL2_ttf。
- SQLite3。
- curl，可打包为本地 `curl.exe`。

Windows/MSYS2 MINGW64 常用包：

```sh
pacman -S mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_ttf mingw-w64-x86_64-sqlite3 mingw-w64-x86_64-curl
```

## 隐私和本地文件

以下内容不应提交：

- `experiment_config.json`：真实 API key。
- `.env` / `deepseek_api_key.txt`：本地密钥。
- `data/`：SQLite 历史数据库，包含故事、选择、prompt 和 raw response。
- `dist/` / `preview/`：打包产物。
- `gb_internovel_runtime.log` 和 `gb_internovel_*_request.json`：运行日志和临时请求。
