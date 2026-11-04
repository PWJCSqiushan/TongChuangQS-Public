# 技术问答

以下回答按当前源码组织，示例账号均为合成演示数据。

1. **为什么让 GUI 和控制台共用核心？** 领域规则放在 `CoreQS`，GUI 与控制台只负责交互，避免同一业务规则在两个入口各自实现。入口分别位于 [`app/gui/main.cpp`](../app/gui/main.cpp)、[`app/console/main.cpp`](../app/console/main.cpp)。
2. **平台和服务如何解耦？** 服务依赖 `ServiceBusQS` 接口传递 `ServiceRequestQS`/`ServiceReplyQS`；`PlatformQS` 实现总线并路由请求。服务之间不直接包含对方实现。
3. **新服务怎样进入平台？** 服务子目录中的 `.cpp` 由 CMake 收集，自注册工厂进入 `ServiceRegistryQS`，平台启动时调用工厂创建服务。新增实现仍需重新编译。
4. **为什么用 OBJECT 库？** `CoreQS` 作为 OBJECT 库把核心对象文件直接带入可执行文件，避免仅含静态自注册对象的目标被链接器丢弃。
5. **服务的通用流程和专属差异分别放在哪里？** `ServiceBaseQS` 负责账号、请求分发和事务骨架；QQ、微信等派生类实现账号编号、能力和服务规则。
6. **好友和群能力如何扩展？** 通过 `FriendCapabilityQS`、`GroupCapabilityQS` 等接口表达可选能力，平台先检查能力是否存在，再发起对应操作。
7. **群策略如何切换？** [`GroupPolicyRegistryQS`](../app/common/GroupPolicyRegistryQS.h) 按策略类型创建实现；QQ、微信、微博策略分别由 [`QQGroupPolicyQS`](../app/common/QQGroupPolicyQS.h)、[`WeChatGroupPolicyQS`](../app/common/WeChatGroupPolicyQS.h)、[`WeiboGroupPolicyQS`](../app/common/WeiboGroupPolicyQS.h) 提供。群对象调用策略规则，而不是把所有平台规则堆进界面。
8. **账号关联如何保存与查询？** 账号保存对端服务与账号的引用；平台请求各服务提供关联结果，再以遍历和去重汇总可达账号。失效关联不会因此自动变成另一账号。
9. **好友推荐和共同好友依据什么？** 平台在关联账号映射可用且服务支持好友能力时，查询来源服务好友，再映射到目标服务并排除自己或已有好友；共同好友则比较两边映射后的集合。
10. **为什么业务状态按服务分目录？** 每项服务拥有独立加载、修订和锁定范围，减少跨服务耦合；相应代价是事务不跨越多个服务。
11. **如何避免部分写入？** `ServiceBaseQS::transact` 在锁内快照内存状态，执行修改后统一提交；保存失败恢复内存副本。`JsonFileQS` 写临时文件后替换目标文件。
12. **revision.json 起什么作用？** 它承载服务修订号及提交后的业务文件快照。其他进程发现修订变化后重新读取，降低并行窗口使用旧状态的机会。
13. **损坏或不完整的数据会怎样？** 加载失败并保留原文件；只有全新数据目录才播种演示数据，避免损坏目录被初始化覆盖。
14. **同一目录能否由多个窗口使用？** 可以读取共享状态；写入时每服务 `.lock` 串行化，成功提交后 revision 促使其他窗口刷新。锁是单服务范围，不是全局数据库事务。
15. **聊天为什么独立于服务总线？** 聊天需要异步 TCP 连接和点对点投递；`ChatNodeQS`、`PeerLinkQS` 负责网络生命周期，服务总线负责同步业务请求路由。
16. **TCP 如何识别完整消息？** [`FrameCodecQS`](../app/chat/FrameCodecQS.cpp) 在 UTF-8 负载前编码四字节网络序长度，接收缓冲区可以处理半帧或多帧，并校验长度上限和负载 UTF-8 编码。JSON 解析由 [`ChatNodeQS`](../app/chat/ChatNodeQS.cpp) 在帧解码后负责。
17. **离线消息如何重试？** 发送端先把消息及待投递状态写入本地历史；连接恢复时重试未确认项，接收端按消息 ID 去重。
18. **ACK 表示什么？** 接收端持久化消息成功后才返回 ACK；这证明落盘，不证明用户已读。发送端再保存确认状态。
19. **身份和通信是否安全认证？** 不是。节点 hello 中的身份是应用层自报；没有 TLS，也没有加密认证，不能在不可信网络中视为安全聊天。
20. **密码摘要是否适合真实账号？** 不适合。`ServiceAccountQS::digest` 是教学实现，不是生产级慢哈希/密钥派生方案；演示账号也不应替换成真实密码。
21. **为什么扩展不是热插件？** CMake 在配置/编译阶段收集服务目录，`-Without` 也会改变编译清单；当前程序没有动态库发现和运行时加载机制。
22. **测试能证明什么，不能证明什么？** CTest 和双进程脚本检查明确的核心、持久化与聊天行为；不能取代第二台真实设备、所有系统版本、极端断电与公网安全测试。历史数字见[测试与限制](TESTING_AND_LIMITS.md)。
23. **注销账号后，号码会分配给新用户吗？** 不会。`AccountBookQS::remove` 将号码放进 `retiredIds_`，`add` 拒绝被保留的号码，数字号码生成器跳过当前账号和退役号码；平台启动时也会保留遗留跨服务关联所引用的缺失号码，避免关联所有权被新账号接管。参见 [`AccountBookQS`](../app/common/AccountBookQS.cpp) 与 [`PlatformQS`](../app/platform/PlatformQS.cpp)。

可直接定位的核心文件：[`PlatformQS`](../app/platform/PlatformQS.h)、[`ServiceBaseQS`](../app/platform/ServiceBaseQS.h)、[`ServiceRegistryQS`](../app/platform/ServiceRegistryQS.h)、[`JsonFileQS`](../app/common/JsonFileQS.h)、[`ServiceAccountQS.cpp`](../app/common/ServiceAccountQS.cpp)、[`ChatNodeQS`](../app/chat/ChatNodeQS.h)、[`FrameCodecQS`](../app/chat/FrameCodecQS.h)、[`ChatHistoryQS`](../app/chat/ChatHistoryQS.h)。
