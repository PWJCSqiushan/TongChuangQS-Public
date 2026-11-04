# 持久化与聊天

## 服务数据

`--data` 指向数据根目录；每项服务使用自己的子目录。首次启动时服务只会在新目录播种演示数据。遇到已有但不完整或损坏的数据时会报错并保留原文件，不会把演示账号静默写进旧目录。

每个服务以 `revision.json` 保存修订号及该服务的业务 JSON 快照。兼容读取旧式分文件数据；成功提交后，快照成为该服务业务数据的权威来源。`ServiceBaseQS::transact` 在写锁下刷新外部修订、保存修改前状态、应用变更、递增修订并提交；失败时恢复内存状态。`JsonFileQS::write` 先写同目录临时文件，再以替换操作提交。锁和快照事务的保证范围是单服务，不是跨服务 ACID 事务。

服务数据目录的 `.lock` 用于阻止两个窗口同时写同一服务目录。`PlatformQS::refreshAll` 检查修订号，发现变化后重载服务状态。裁剪构建若面对包含已禁用服务数据的目录会拒绝启动，以免缺少对应服务的构建误伤账号关联。

账号注销时，[`AccountBookQS`](../app/common/AccountBookQS.cpp) 会把号码放入已退役集合；号码分配和新增校验都会跳过/拒绝这些号码。启动时 [`PlatformQS`](../app/platform/PlatformQS.cpp) 还会从跨服务遗留关联中保留缺失账号号码，避免旧关联被新注册账号重新占用。

## TCP 聊天

聊天由 [`ChatNodeQS`](../app/chat/ChatNodeQS.h)、[`SocketQS`](../app/chat/SocketQS.h)、[`PeerLinkQS`](../app/chat/PeerLinkQS.h) 和 [`FrameCodecQS`](../app/chat/FrameCodecQS.h) 组成。节点通过 TCP 连接交换 hello、身份和消息；帧使用四字节网络序长度前缀及 UTF-8 文本负载。`FrameCodecQS` 检查帧长度上限并验证负载是否为合法 UTF-8；`ChatNodeQS` 随后解析负载中的 JSON，两层职责不同。聊天独立于服务请求总线。

发送方先把待投递消息写入自己的聊天历史，再尝试投递。接收方以消息 ID 去重并先持久化收到的消息，再回 ACK；发送方记录 ACK 状态，未确认消息会在重连后重试。因此，ACK 表示接收端已落盘，不表示用户已读。聊天记录按节点/账号保存在本地 JSON 文件中；在线状态和端点用于发现同一数据目录内的节点。

默认节点仅监听回环地址；`--lan` 才请求局域网监听。身份是应用层自报，不是加密认证；连接没有 TLS。此实现用于本地课程演示，不能作为生产聊天服务。

源码定位：[`JsonFileQS`](../app/common/JsonFileQS.cpp)、[`ServiceBaseQS`](../app/platform/ServiceBaseQS.cpp)、[`PlatformQS`](../app/platform/PlatformQS.cpp)、[`ChatNodeQS`](../app/chat/ChatNodeQS.cpp)、[`PeerLinkQS`](../app/chat/PeerLinkQS.cpp)、[`FrameCodecQS`](../app/chat/FrameCodecQS.cpp)、[`ChatHistoryQS`](../app/chat/ChatHistoryQS.cpp)、[`PresenceBoardQS`](../app/chat/PresenceBoardQS.cpp)。
