# 测试与限制

## 可重复的测试入口

在构建命令后加 `-Test` 会运行 CTest。完整构建包含 `service-smoke`、`core`、`chat`、`delivery-regression`；裁剪服务构建主要运行 `service-smoke`。测试源位于 [`app/tests/`](../app/tests/)。另有控制台双进程 E2E 脚本 [`app/scripts/console_two_window_e2e.py`](../app/scripts/console_two_window_e2e.py)，GUI 聊天自检入口为 `--chat-selftest A/B`。

## 2026-11-04 本轮结果

隔离便携副本的可执行文件 SHA 与正式版一致。GUI 30 项检查全部通过；双进程 TCP 与消息落盘 ACK 自检通过。聊天自检在默认受限执行环境下未通过，在常规 Windows 用户上下文复核后通过；两份证据均保存在本地。

CMake 3.31.6-msvc6 在配置阶段以 `0xC0000409` 退出，没有生成可用构建，因此本轮四套 CTest 未运行。此次检查没有修改功能源码或真实数据目录，也未为归档安装或修复大型 SDK。本轮结果不包含四套 CTest 的通过结论。

## 历史验证记录

2026-10-23 有四套测试合计 372 项检查；裁剪微商服务构建有 32 项检查；GUI 完整构建有 30 项、裁剪构建有 27 项检查。2026-10-28 的恢复自检记录为 30 项检查加 TCP 检查。这些数字都是对应日期的历史结果，不是 2026-11-04 本轮成绩。

自动测试覆盖确定的领域规则、JSON 故障回滚、跨进程刷新、TCP 分帧、重试和 ACK 落盘等行为。它不能证明所有 Windows 版本、网络环境、断电时序和真实设备组合均正常。未完成第二台实机、Windows 10 全面兼容和断电测试的结论，不在此宣称已验证。

## 安全与运行边界

- QQ、微信、微博、微商只是课程模拟服务，没有连接商业平台。
- 聊天身份由应用层声明，不构成加密认证；没有 TLS。
- 密码摘要是教学用途实现，不是生产级密码存储算法；不能用于保护真实凭据。
- ACK 是接收端落盘确认，不是已读回执。
- 写入事务以单个服务为边界，不提供跨服务 ACID 保证。
- 服务发现和裁剪在编译期完成，不支持运行时热插件。
- 本项目使用 JSON 文件，而非数据库；它适合本地演示，不是高并发或多机部署方案。

相关实现：[`app/tests/`](../app/tests/)、[`ServiceAccountQS.cpp`](../app/common/ServiceAccountQS.cpp)、[`JsonFileQS.cpp`](../app/common/JsonFileQS.cpp)、[`ServiceBaseQS.cpp`](../app/platform/ServiceBaseQS.cpp)、[`FrameCodecQS.cpp`](../app/chat/FrameCodecQS.cpp) 与 [`ChatNodeQS.cpp`](../app/chat/ChatNodeQS.cpp)。
