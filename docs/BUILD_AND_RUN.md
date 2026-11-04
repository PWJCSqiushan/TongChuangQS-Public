# 构建与运行

## 环境

- Windows 10/11 x64。
- Visual Studio 2022，安装“使用 C++ 的桌面开发”与 CMake 工具。
- GUI 构建需要 Qt 6.8.x `msvc2022_64`；仅构建控制台时可省略 Qt。
- CMake 入口为 [`app/CMakeLists.txt`](../app/CMakeLists.txt)，语言标准为 C++17。

## 构建

在仓库根目录打开 PowerShell。GUI 版：

```powershell
.\BuildQS.cmd -QtRoot "<Qt 安装目录>\6.8.x\msvc2022_64" -Test
```

控制台版：

```powershell
.\BuildQS.cmd -NoGui -Test
```

`-Test` 会构建并运行 CTest。`-Without weshop` 是编译期服务裁剪演示，例如 `.\BuildQS.cmd -NoGui -Without weshop -Test`。服务目录通过 CMake 收集并参与构建；裁剪不会变成运行时热插件。构建脚本会在需要时为含非 ASCII 字符的源码路径创建 ASCII 目录联接，临时目录也需使用 ASCII 路径。

构建成功后，程序和 GUI 运行文件位于仓库根目录的 `runtime`。使用 `-Without weshop` 时，输出目录为 `runtime-without-weshop`。根目录启动器固定指向 `runtime`，不会自动切到裁剪版；要演示裁剪版，请从仓库根目录直接运行对应目录中的程序，例如 `.\runtime-without-weshop\SocialQS.exe --data '.\demo-data'`，或将程序名换成 `TongChuangQS.exe` 启动 GUI，并为裁剪版指定独立数据目录。裁剪构建遇到含有已禁用服务数据的目录会拒绝启动。脚本不会自动删除构建缓存；如需清理，请先把对应目录移入回收站。

## 启动与数据目录

在仓库根目录运行 [`.\StartQS.cmd`](../StartQS.cmd) 启动 GUI，或 [`.\StartConsoleQS.cmd`](../StartConsoleQS.cmd) 启动控制台。两个启动器都默认把 `data` 传给程序；也可从命令行直接运行程序并通过 `--data <目录>` 指定目录。GUI 入口还接受 `--window 1|2`，聊天节点支持 `--lan`。双窗口聊天演示入口为 [`.\StartChatDemoQS.cmd`](../StartChatDemoQS.cmd)。

首次运行会在所选数据目录生成演示数据；此后同一目录用于持久化账号、服务数据与聊天记录。两个进程可共享一个目录并使用不同聊天账号；服务数据锁和修订号用于串行化写入并刷新对方的修改。同一个聊天账号同时由另一窗口持有时，该账号身份锁会阻止重复登录。请不要把不同版本的旧数据目录用作首次运行示范。

## 本地演示

演示数据可使用 QQ 账号 `10001`、`10002`，密码 `123456`。这些值仅用于本地演示，不能替换为真实账号或个人资料。它们不是外部平台账号。需要两个聊天端时，使用 [`.\StartChatDemoQS.cmd`](../StartChatDemoQS.cmd)，分别登录这两个演示账号并在同一数据目录中操作。

控制台二进程端到端脚本位于 [`app/scripts/console_two_window_e2e.py`](../app/scripts/console_two_window_e2e.py)；GUI 聊天自检入口为 `--chat-selftest A/B`。构建选项见 [`app/scripts/build.ps1`](../app/scripts/build.ps1)。具体测试范围见[测试与限制](TESTING_AND_LIMITS.md)。
