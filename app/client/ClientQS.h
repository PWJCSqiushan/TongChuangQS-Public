#pragma once

#include "ChatNodeQS.h"
#include "PlatformQS.h"

#include <memory>
#include <string>
#include <vector>

#include "ConversationViewQS.h"

namespace qs {

/** 会话列表中的一项（已合并最新的昵称/群名）。 */


/**
 * 一个窗口（客户端）的应用层：组合平台 PlatformQS 与聊天节点 ChatNodeQS。
 * 控制台和图形界面共用这一层，界面只负责显示与输入。
 *  - 登录支持聊天的服务时自动在聊天节点上线；退出时下线；
 *  - 收信校验：私聊只收好友的，群聊只收同群成员的；
 *  - 发送群消息时从群能力取成员名单做扇出。
 */
class ClientQS {
public:
    explicit ClientQS(std::string dataRootUtf8);
    ~ClientQS();

    ResultQS start(bool allowLan = false);
    PlatformQS& platform() { return *platform_; }
    const PlatformQS& platform() const { return *platform_; }
    ChatNodeQS& chat() { return *chat_; }

    ResultQS login(const std::string& service, const std::string& account, const std::string& password);
    ResultQS confirmPending(const std::string& service);
    void logout(const std::string& service);
    void logoutAll();
    std::string accountOf(const std::string& service) const;
    AccountRefQS me(const std::string& service) const;

    // ———— 聊天 ————
    bool chatReady(const std::string& service) const;
    std::vector<ConversationViewQS> conversations(const std::string& service);
    ChatHistoryQS* history(const std::string& service);
    ChatMessageQS sendToFriend(const std::string& service, const std::string& friendAccount, const std::string& text);
    ResultQS sendToGroup(const std::string& service, const std::string& groupId, const std::string& text, ChatMessageQS& out);
    bool isOnline(const std::string& service, const std::string& account) const;
    std::string displayNameOf(const std::string& service, const std::string& account) const;

    /** 定时调用：处理网络消息、心跳与补发、读取其他窗口对数据的修改。返回聊天事件。 */
    std::vector<ChatEventQS> pump(bool& dataChanged);

private:
    ResultQS bringChatOnline(const std::string& service);
    bool acceptIncoming(const AccountRefQS& owner, const ChatMessageQS& message) const;

    std::unique_ptr<PlatformQS> platform_;
    std::unique_ptr<ChatNodeQS> chat_;
    std::int64_t lastTick_ = 0;
    std::int64_t lastRefresh_ = 0;
};

}  // namespace qs
