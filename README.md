# 同窗 QS（TongChuangQS）

同窗 QS 是一个以 C++17 编写的多服务社交模拟课设，包含 QQ、微信、微博和微商四种课程服务。它展示账号关联、好友与群组领域模型、JSON 本地持久化，以及可选的 Qt 桌面界面和 TCP 聊天。这里的 QQ、微信、微博和微商均为课程模拟服务，不连接相应商业平台。

## 构建与运行

源码包没有预编译程序。Windows x64 构建需要用户自行安装 Visual Studio 2022 的 C++ 桌面开发工具（含 CMake）；构建图形界面还需要自行安装 Qt 6.8.x 的 `msvc2022_64` 组件。控制台构建可使用 `-NoGui`，不需要 Qt。解压到可写目录后，在 PowerShell 中运行：

```powershell
.\BuildQS.cmd -QtRoot "<Qt 6.8.x 安装目录>\msvc2022_64" -Test
```

不构建 GUI 时运行 `.\BuildQS.cmd -NoGui -Test`。成功后构建脚本在仓库根目录生成 `runtime`，再运行 `.\StartQS.cmd`（GUI）或 `.\StartConsoleQS.cmd`（控制台）。详细步骤见[构建与运行](docs/BUILD_AND_RUN.md)。

## 文档

- [架构概览](docs/ARCHITECTURE.md)：模块边界、服务扩展和核心流程。
- [持久化与聊天](docs/PERSISTENCE_AND_CHAT.md)：JSON 提交、跨进程刷新和 TCP 消息确认。
- [用户指南](docs/USER_GUIDE.md)：首次启动、演示账号、服务和聊天操作。
- [测试与限制](docs/TESTING_AND_LIMITS.md)：历史验证口径、测试入口与已知边界。
- [技术问答](docs/TECHNICAL_QA.md)：围绕设计和当前源码的常见答辩问题。

## 公开范围与授权

本仓库只包含源码和技术文档，不包含预编译程序、个人课程原件或私人日志/截图。项目没有附带项目级开源许可证，因此这里不作项目授权声明；使用时应自行确认适用权利。第三方组件各自的许可条款仍然适用，且应保留随源码提供的许可文件。

公开作者名：PWJCSqiushan。
