请参考 LPF970915/ROCreader 的技术架构，为我们的复古掌机 App 创建一个新的 C++17 + SDL2 项目骨架。

要求：

1. 不要复制 ROCreader 的漫画阅读业务代码。
2. 只参考其工程架构：
   - Makefile 模块分组
   - SDL2 fullscreen/windowed 运行时
   - AppContext
   - SceneManager
   - InputManager
   - LayoutMetrics
   - native_config.ini
   - native_keymap.ini
   - /Roms/APPS 打包方式
3. 项目目标设备为 H700 / RG35XX 类 Linux 掌机，同时支持 Windows 桌面预览。
4. UI 逻辑分辨率固定为 720x480，所有页面按 3:2 横屏设计。
5. 输入方式以 D-pad、A/B/X/Y、L1/R1、Start、Select、Menu 为主。
6. 第一阶段只实现空壳：
   - BootScene
   - HomeScene
   - SettingsScene
   - SessionScene
7. 架构上预留 business 层，后续用于 AI 互动小说或节拍器等应用。
8. 输出可编译项目，包含：
   - src/
   - assets/
   - Makefile
   - native_config.ini
   - native_keymap.ini
   - build_and_run.sh
   - package_to_apps.sh
9. 不引入 PDF、EPUB、ZIP、MuPDF、Poppler 等阅读器相关依赖。
10. 代码风格保持简单、低依赖、适合低性能 Linux 掌机。