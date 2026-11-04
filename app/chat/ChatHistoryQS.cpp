#include "ChatHistoryQS.h"

#include "JsonFileQS.h"

#include <algorithm>

namespace qs {


ChatHistoryQS::ChatHistoryQS(std::string owner, std::filesystem::path file) : owner_(std::move(owner)), file_(std::move(file)) {
    load();
}

ChatHistoryQS::~ChatHistoryQS() { save(); }

bool ChatHistoryQS::append(const ChatMessageQS& message, const std::string& title, bool countUnread) {
    if (find(message.id) != nullptr) return false;
    ConversationQS& c = ensureConversation(message.conversationKey(owner_), title);
    c.messages.push_back(message);
    if (countUnread) ++c.unread;
    return true;
}

bool ChatHistoryQS::appendDurable(const ChatMessageQS& message, const std::string& title, bool countUnread) {
    if (message.id.empty()) return false;
    if (auto* existing = find(message.id)) return existing->toWire() == message.toWire() && save();
    const auto before = conversations_;
    append(message, title, countUnread);
    if (save()) return true;
    conversations_ = before;
    return false;
}

ChatMessageQS* ChatHistoryQS::find(const std::string& messageId) {
    for (auto& c : conversations_) {
        for (auto it = c.second.messages.rbegin(); it != c.second.messages.rend(); ++it) {
            if (it->id == messageId) return &*it;
        }
    }
    return nullptr;
}

ConversationQS* ChatHistoryQS::conversation(const std::string& key) {
    const auto it = conversations_.find(key);
    return it == conversations_.end() ? nullptr : &it->second;
}

ConversationQS& ChatHistoryQS::ensureConversation(const std::string& key, const std::string& title) {
    ConversationQS& c = conversations_[key];
    c.key = key;
    if (!title.empty()) c.title = title;
    return c;
}

std::vector<const ConversationQS*> ChatHistoryQS::conversations() const {
    std::vector<const ConversationQS*> result;
    for (const auto& c : conversations_) result.push_back(&c.second);
    std::sort(result.begin(), result.end(),
              [](const ConversationQS* a, const ConversationQS* b) { return a->lastTime() > b->lastTime(); });
    return result;
}

void ChatHistoryQS::markRead(const std::string& key) {
    if (ConversationQS* c = conversation(key)) c->unread = 0;
}

int ChatHistoryQS::unreadTotal() const {
    int total = 0;
    for (const auto& c : conversations_) total += c.second.unread;
    return total;
}

std::vector<ChatSearchHitQS> ChatHistoryQS::search(const std::string& keyword) const {
    std::vector<ChatSearchHitQS> hits;
    if (keyword.empty()) return hits;
    for (const auto& c : conversations_) {
        for (const auto& m : c.second.messages) {
            if (m.text.find(keyword) != std::string::npos) hits.push_back(ChatSearchHitQS{c.first, c.second.title, m});
        }
    }
    std::sort(hits.begin(), hits.end(),
              [](const ChatSearchHitQS& a, const ChatSearchHitQS& b) { return a.message.time > b.message.time; });
    return hits;
}

std::vector<ChatMessageQS*> ChatHistoryQS::outbox() {
    std::vector<ChatMessageQS*> result;
    for (auto& c : conversations_) {
        for (auto& m : c.second.messages) {
            if (m.outgoing && !m.pendingTo.empty()) result.push_back(&m);
        }
    }
    return result;
}

void ChatHistoryQS::clearConversation(const std::string& key) { conversations_.erase(key); }

void ChatHistoryQS::load() {
    conversations_.clear();
    const JsonQS root = JsonFileQS::read(file_);
    for (const auto& c : root.value("conversations", JsonQS::array())) {
        ConversationQS conv;
        conv.key = c.value("key", std::string());
        conv.title = c.value("title", std::string());
        conv.unread = c.value("unread", 0);
        for (const auto& m : c.value("messages", JsonQS::array())) conv.messages.push_back(ChatMessageQS::fromJson(m));
        if (!conv.key.empty()) conversations_[conv.key] = std::move(conv);
    }
}

bool ChatHistoryQS::save() const {
    JsonQS list = JsonQS::array();
    for (const auto& c : conversations_) {
        JsonQS messages = JsonQS::array();
        for (const auto& m : c.second.messages) messages.push_back(m.toJson());
        list.push_back(JsonQS{{"key", c.first}, {"title", c.second.title}, {"unread", c.second.unread}, {"messages", messages}});
    }
    return JsonFileQS::write(file_, JsonQS{{"owner", owner_}, {"conversations", list}});
}

}  // namespace qs
