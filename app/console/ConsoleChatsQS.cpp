#include "ConsoleChatsQS.h"

#include "DateQS.h"

namespace qs {

namespace {

std::string formatLine(const ChatMessageQS& m) {
    std::string who = m.outgoing ? "我" : m.fromNick;
    std::string line = "  [" + DateQS::clockText(m.time) + "] " + who + "：" + m.text;
    if (m.outgoing) line += "   (" + m.statusText() + ")";
    return line;
}

/** 进入会话：先显示最近记录，然后逐行发送；输入 /q 退出。新消息由后台线程实时打印。 */
void chatLoop(ConsoleContextQS& c, const std::string& key, const std::string& peer, bool group, const std::string& title) {
    {
        std::lock_guard<std::recursive_mutex> guard(c.lock);
        c.io.title("与 " + title + " 的对话（输入 /q 返回）");
        if (ChatHistoryQS* h = c.client.history(c.service)) {
            if (ConversationQS* conv = h->conversation(key)) {
                const std::size_t start = conv->messages.size() > 15 ? conv->messages.size() - 15 : 0;
                for (std::size_t i = start; i < conv->messages.size(); ++i) c.io.print(formatLine(conv->messages[i]));
            }
            h->markRead(key);
        }
        if (!group && !c.client.isOnline(c.service, peer)) c.io.print("  （对方当前离线，消息将在对方上线后自动送达）");
        c.activeConversation = key;
    }
    while (!c.io.eof()) {
        const std::string text = c.io.readLine("");
        if (text == "/q") break;
        if (text.empty()) continue;
        std::lock_guard<std::recursive_mutex> guard(c.lock);
        if (group) {
            ChatMessageQS sent;
            const ResultQS r = c.client.sendToGroup(c.service, peer, text, sent);
            if (!r) c.io.warn(r.message());
            else c.io.print(formatLine(sent));
        } else {
            c.io.print(formatLine(c.client.sendToFriend(c.service, peer, text)));
        }
    }
    std::lock_guard<std::recursive_mutex> guard(c.lock);
    c.activeConversation.clear();
}

}  // namespace

void ConsoleChatsQS::run(ConsoleContextQS& c) {
    while (!c.io.eof()) {
        std::vector<ConversationViewQS> list;
        {
            std::lock_guard<std::recursive_mutex> guard(c.lock);
            if (!c.client.chatReady(c.service)) {
                c.io.warn("聊天未就绪：该账号可能已在另一个窗口登录");
                return;
            }
            list = c.client.conversations(c.service);
            c.io.title(c.svc().name() + " · 消息（本机端口 " + std::to_string(c.client.chat().port()) + "）");
            for (std::size_t i = 0; i < list.size(); ++i) {
                const auto& v = list[i];
                std::string line = "  " + std::to_string(i + 1) + ". " + (v.isGroup ? "[群] " : "") + v.title;
                if (!v.isGroup) line += v.online ? " ●" : " ○";
                if (v.unread > 0) line += "  (" + std::to_string(v.unread) + "条未读)";
                c.io.print(line + "  " + DateQS::shortText(v.time) + "  " + v.preview);
            }
            if (list.empty()) c.io.print("  还没有聊天记录");
            c.io.print("  —— 输入会话序号进入；a. 找好友聊天  g. 进入群聊  s. 搜索聊天记录  0. 返回");
        }
        const std::string pick = c.io.readLine("请选择：");
        if (pick == "0" || c.io.eof()) return;
        if (pick == "a") {
            std::vector<FriendViewQS> friends;
            {
                std::lock_guard<std::recursive_mutex> guard(c.lock);
                friends = c.svc().friendCapability()->friendsOf(c.me());
                for (std::size_t i = 0; i < friends.size(); ++i) {
                    c.io.print("  " + std::to_string(i + 1) + ". " + friends[i].displayName() + "（" + friends[i].account + "）" +
                               (c.client.isOnline(c.service, friends[i].account) ? " ● 在线" : " ○ 离线"));
                }
            }
            if (friends.empty()) continue;
            const int n = c.io.readChoice("和谁聊天（0 返回）：", 0, static_cast<int>(friends.size()));
            if (n > 0) {
                const auto& f = friends[static_cast<std::size_t>(n - 1)];
                chatLoop(c, "p:" + f.account, f.account, false, f.displayName());
            }
        } else if (pick == "g") {
            std::vector<GroupQS> groups;
            {
                std::lock_guard<std::recursive_mutex> guard(c.lock);
                groups = c.svc().groupCapability()->groupsOf(c.me());
                for (std::size_t i = 0; i < groups.size(); ++i) {
                    c.io.print("  " + std::to_string(i + 1) + ". " + groups[i].name() + "（" + std::to_string(groups[i].members().size()) + "人）");
                }
            }
            if (groups.empty()) continue;
            const int n = c.io.readChoice("进入哪个群（0 返回）：", 0, static_cast<int>(groups.size()));
            if (n > 0) {
                const auto& g = groups[static_cast<std::size_t>(n - 1)];
                chatLoop(c, "g:" + g.id(), g.id(), true, g.name());
            }
        } else if (pick == "s") {
            const std::string key = c.io.readLine("搜索内容：");
            std::lock_guard<std::recursive_mutex> guard(c.lock);
            const auto hits = c.client.history(c.service)->search(key);
            c.io.print("  找到 " + std::to_string(hits.size()) + " 条：");
            for (const auto& h : hits) c.io.print("  [" + h.conversationTitle + "] " + DateQS::dateText(h.message.time) + formatLine(h.message));
        } else {
            try {
                const std::size_t n = static_cast<std::size_t>(std::stoi(pick));
                if (n >= 1 && n <= list.size()) {
                    const auto& v = list[n - 1];
                    chatLoop(c, v.key, v.peer, v.isGroup, v.title);
                }
            } catch (...) {
                c.io.warn("无效的选择");
            }
        }
    }
}

QS_REGISTER_CONSOLE_FEATURE(ConsoleChatsQS, "chats")

}  // namespace qs
