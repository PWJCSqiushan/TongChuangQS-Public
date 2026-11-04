#pragma once

#include "ChatMessageQS.h"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "ConversationQS.h"
#include "ChatSearchHitQS.h"

namespace qs {

/** 一个会话（与某人私聊，或某个群）。 */


/** 搜索结果。 */


/**
 * 某个账号的全部聊天记录（容器）。保存在 data/<服务>/chat/<账号>.json，
 * 只由登录了该账号的窗口读写，实例化时读入、析构时写回。
 */
class ChatHistoryQS {
public:
    ChatHistoryQS(std::string owner, std::filesystem::path file);
    ~ChatHistoryQS();
    ChatHistoryQS(const ChatHistoryQS&) = delete;
    ChatHistoryQS& operator=(const ChatHistoryQS&) = delete;

    const std::string& owner() const noexcept { return owner_; }

    /** 追加消息；已存在同 id 的消息时忽略并返回 false（防止重发造成重复）。 */
    bool append(const ChatMessageQS& message, const std::string& title, bool countUnread);
    /** Commit before acknowledging; duplicates of the same wire message are accepted. */
    bool appendDurable(const ChatMessageQS& message, const std::string& title, bool countUnread);
    ChatMessageQS* find(const std::string& messageId);
    ConversationQS* conversation(const std::string& key);
    ConversationQS& ensureConversation(const std::string& key, const std::string& title);
    /** 按最后一条消息时间倒序。 */
    std::vector<const ConversationQS*> conversations() const;
    void markRead(const std::string& key);
    int unreadTotal() const;
    std::vector<ChatSearchHitQS> search(const std::string& keyword) const;
    /** 还有待发送接收者的外发消息（用于上线补发）。 */
    std::vector<ChatMessageQS*> outbox();
    void clearConversation(const std::string& key);

    void load();
    bool save() const;

private:
    std::string owner_;
    std::filesystem::path file_;
    std::map<std::string, ConversationQS> conversations_;
};

}  // namespace qs
