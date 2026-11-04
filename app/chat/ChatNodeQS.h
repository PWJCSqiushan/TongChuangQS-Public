#pragma once

#include "AccountRefQS.h"
#include "ChatHistoryQS.h"
#include "FileLockQS.h"
#include "NetworkRuntimeQS.h"
#include "PeerLinkQS.h"
#include "PresenceBoardQS.h"
#include "ResultQS.h"

#include <atomic>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "ChatEventQS.h"

namespace qs {

/** 聊天节点产生的事件，界面据此刷新。 */


/**
 * 聊天节点：一个窗口（进程）一个节点，负责该窗口内所有已登录聊天账号的点对点通信。
 *
 *  - 启动时在本机监听一个由系统分配的端口（accept 线程）；
 *  - 账号登录后在在线公告板登记"账号→端口"，好友据此自动连接，无需手填地址；
 *  - 发送：查到对方在线即建立/复用 TCP 连接并发出消息帧；对方不在线则保存为待发送，
 *    对方上线后 tick() 自动补发（离线消息）；
 *  - 接收：读线程把帧放入收件队列，poll() 在界面线程中取出、写入聊天记录、回送回执；
 *  - 群聊：服务端不存在，发送方向每位在线成员各发一份（扇出），逐个统计回执。
 */
class ChatNodeQS {
public:
    using ValidatorQS = std::function<bool(const AccountRefQS& owner, const ChatMessageQS& message)>;

    explicit ChatNodeQS(const std::string& dataRootUtf8);
    ~ChatNodeQS();
    ChatNodeQS(const ChatNodeQS&) = delete;
    ChatNodeQS& operator=(const ChatNodeQS&) = delete;

    /** allowLan=true 时监听所有网卡，允许局域网内其他电脑连接。 */
    ResultQS start(bool allowLan = false);
    void stop();
    bool running() const noexcept { return running_; }
    unsigned short port() const noexcept { return port_; }

    /** 设置收信校验（例如：只接收好友或同群成员的消息）。 */
    void setValidator(ValidatorQS validator) { validator_ = std::move(validator); }

    ResultQS addIdentity(const AccountRefQS& me, const std::string& nickname, const std::filesystem::path& historyFile);
    void removeIdentity(const AccountRefQS& me);
    bool hasIdentity(const AccountRefQS& me) const { return identities_.count(me) != 0; }
    ChatHistoryQS* history(const AccountRefQS& me);

    bool isOnline(const AccountRefQS& peer) const;
    /** 手动连接另一台电脑上的节点（局域网演示用）。 */
    ResultQS connectTo(const std::string& host, unsigned short port);

    ChatMessageQS sendPrivate(const AccountRefQS& me, const std::string& peer, const std::string& peerNick,
                              const std::string& text);
    ChatMessageQS sendGroup(const AccountRefQS& me, const std::string& groupId, const std::string& groupName,
                            const std::vector<std::string>& members, const std::string& text);

    /** 在界面线程中调用：处理收到的网络消息，返回需要界面响应的事件。 */
    std::vector<ChatEventQS> poll();
    /** 定时调用（约每秒）：发送在线心跳、给刚上线的好友补发离线消息。 */
    std::vector<ChatEventQS> tick();

private:
#include "IdentityQS.h"

    void acceptLoop();
    int addLink(SocketQS socket);
    int linkFor(const AccountRefQS& target);
    bool sendOnLink(int linkId, const std::string& payload);
    void sendHello(int linkId, bool reply);
    void broadcastRoster();
    bool deliver(ChatMessageQS& message, std::vector<ChatEventQS>* events);
    void handleFrame(int linkId, const std::string& payload, std::vector<ChatEventQS>& events);
    void purgeDeadLinks(std::vector<ChatEventQS>& events);
    std::string nodeEndpoint() const;
    std::string nextMessageId(const AccountRefQS& me);

    NetworkRuntimeQS runtime_;
    PresenceBoardQS presence_;
    std::filesystem::path identityLockRoot_;
    SocketQS listener_;
    std::thread acceptThread_;
    std::atomic<bool> running_{false};
    unsigned short port_ = 0;
    long long pid_ = 0;
    ValidatorQS validator_;

    mutable std::mutex linksMutex_;
    std::map<int, std::unique_ptr<PeerLinkQS>> links_;
    std::map<std::string, int> endpointLinks_;
    std::map<int, std::set<AccountRefQS>> linkIdentities_;
    int nextLinkId_ = 1;

    std::mutex inboxMutex_;
    std::deque<std::pair<int, std::string>> inbox_;

    std::map<AccountRefQS, IdentityQS> identities_;
    std::map<AccountRefQS, std::string> remoteRoster_;  // 经网络得知的远端账号 → 节点
    std::int64_t lastHeartbeat_ = 0;
    long long messageCounter_ = 0;
};

}  // namespace qs
