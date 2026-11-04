#pragma once

#include <QString>

namespace qs {

class MainWindowQS;

/**
 * 双进程聊天自检（开发与验收辅助）。
 * orchestrate() 启动两个独立的 TongChuangQS.exe 窗口（共用一个全新的数据目录），
 * 分别以 QQ 10001 / 10002 登录；每个窗口在真实的输入框里输入文字、按 Enter 发送，
 * 并检查对方的消息以气泡形式出现在聊天区、自己的消息收到送达回执。结果写入 JSON。
 */
class ChatSelfTestQS {
public:
    /** 在当前进程中扮演一方：role 为 "A" 或 "B"。返回 0 表示通过。 */
    static int runRole(MainWindowQS& window, const QString& role, const QString& resultFile);
    /** 启动两个子进程并汇总结果到 summaryFile。返回是否全部通过。 */
    static bool orchestrate(const QString& exePath, const QString& workDir, const QString& summaryFile);
};

}  // namespace qs
