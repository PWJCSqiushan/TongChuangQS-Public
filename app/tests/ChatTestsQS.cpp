// 聊天模块测试：多个 ChatNodeQS 在同一台机器上通过真实 TCP 连接互发消息。
#include "ChatNodeQS.h"
#include "PathQS.h"
#include "TestHarnessQS.h"

#include <chrono>
#include <filesystem>
#include <functional>
#include <thread>

using namespace qs;

namespace {

std::string freshDir(const std::string& name) {
    const auto dir = std::filesystem::temp_directory_path() /
                     ("qs-chat-" + name + "-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(dir);
    return PathQS::toUtf8(dir);
}

std::filesystem::path historyFile(const std::string& root, const std::string& account) {
    return PathQS::fromUtf8(root) / "qq" / "chat" / (account + ".json");
}

/** 反复 poll/tick 直到条件成立或超时。 */
bool pump(std::vector<ChatNodeQS*> nodes, const std::function<bool()>& done, int timeoutMs = 4000) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        for (ChatNodeQS* n : nodes) {
            n->poll();
            n->tick();
        }
        if (done()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}

const ChatMessageQS* lastIn(ChatNodeQS& node, const AccountRefQS& me, const std::string& key) {
    ChatHistoryQS* h = node.history(me);
    if (h == nullptr) return nullptr;
    ConversationQS* c = h->conversation(key);
    return (c == nullptr || c->messages.empty()) ? nullptr : &c->messages.back();
}

}  // namespace

QS_TEST(PrivateMessageWithDeliveryReceipt) {
    const std::string root = freshDir("private");
    ChatNodeQS a(root), b(root);
    QS_CHECK(a.start());
    QS_CHECK(b.start());
    const AccountRefQS qiushan("qq", "10001"), linran("qq", "10002");
    QS_CHECK(a.addIdentity(qiushan, "丘山", historyFile(root, "10001")));
    QS_CHECK(b.addIdentity(linran, "林然", historyFile(root, "10002")));
    QS_CHECK(a.isOnline(linran));
    const ChatMessageQS sent = a.sendPrivate(qiushan, "10002", "林然", "你好，晚上一起彩排？");
    QS_CHECK(sent.status == MessageStatusQS::Sent);
    QS_CHECK(pump({&a, &b}, [&] {
        const ChatMessageQS* got = lastIn(b, linran, "p:10001");
        const ChatMessageQS* mine = lastIn(a, qiushan, "p:10002");
        return got != nullptr && mine != nullptr && mine->status == MessageStatusQS::Delivered;
    }));
    const ChatMessageQS* got = lastIn(b, linran, "p:10001");
    QS_CHECK(got != nullptr && got->text == "你好，晚上一起彩排？" && !got->outgoing);
    QS_CHECK(b.history(linran)->unreadTotal() == 1);
    // 回复
    b.sendPrivate(linran, "10001", "丘山", "好的，八点见");
    QS_CHECK(pump({&a, &b}, [&] {
        const ChatMessageQS* m = lastIn(a, qiushan, "p:10002");
        return m != nullptr && m->text == "好的，八点见";
    }));
}

QS_TEST(OfflineMessageDeliveredWhenPeerComesOnline) {
    const std::string root = freshDir("offline");
    ChatNodeQS a(root);
    QS_CHECK(a.start());
    const AccountRefQS me("qq", "10001"), peer("qq", "10003");
    QS_CHECK(a.addIdentity(me, "丘山", historyFile(root, "10001")));
    QS_CHECK(!a.isOnline(peer));
    const ChatMessageQS m = a.sendPrivate(me, "10003", "许知", "看到回我");
    QS_CHECK(m.status == MessageStatusQS::Pending);
    ChatNodeQS c(root);
    QS_CHECK(c.start());
    QS_CHECK(c.addIdentity(peer, "许知", historyFile(root, "10003")));
    QS_CHECK(pump({&a, &c}, [&] {
        const ChatMessageQS* got = lastIn(c, peer, "p:10001");
        const ChatMessageQS* mine = lastIn(a, me, "p:10003");
        return got != nullptr && mine != nullptr && mine->status == MessageStatusQS::Delivered;
    }));
}

QS_TEST(GroupFanOutAndReceipts) {
    const std::string root = freshDir("group");
    ChatNodeQS a(root), b(root), c(root);
    QS_CHECK(a.start() && b.start() && c.start());
    const AccountRefQS r1("qq", "10001"), r2("qq", "10002"), r3("qq", "10003");
    a.addIdentity(r1, "丘山", historyFile(root, "10001"));
    b.addIdentity(r2, "林然", historyFile(root, "10002"));
    c.addIdentity(r3, "许知", historyFile(root, "10003"));
    const ChatMessageQS m = a.sendGroup(r1, "1001", "C++课程设计交流群", {"10001", "10002", "10003", "10005"}, "大家好");
    QS_CHECK(m.recipients == 3);
    QS_CHECK(pump({&a, &b, &c}, [&] {
        const ChatMessageQS* mine = lastIn(a, r1, "g:1001");
        return lastIn(b, r2, "g:1001") != nullptr && lastIn(c, r3, "g:1001") != nullptr && mine != nullptr &&
               mine->ackedBy.size() == 2;
    }));
    const ChatMessageQS* mine = lastIn(a, r1, "g:1001");
    QS_CHECK(mine->pendingTo.size() == 1);  // 10005 离线，待补发
    QS_CHECK(mine->statusText().find("2/3") != std::string::npos);
    QS_CHECK(lastIn(b, r2, "g:1001")->fromNick == "丘山");
}

QS_TEST(HistoryPersistsAndIsSearchable) {
    const std::string root = freshDir("history");
    {
        ChatNodeQS a(root), b(root);
        a.start();
        b.start();
        a.addIdentity(AccountRefQS("qq", "10001"), "丘山", historyFile(root, "10001"));
        b.addIdentity(AccountRefQS("qq", "10002"), "林然", historyFile(root, "10002"));
        a.sendPrivate(AccountRefQS("qq", "10001"), "10002", "林然", "答辩PPT我放群文件了");
        a.sendPrivate(AccountRefQS("qq", "10001"), "10002", "林然", "记得带U盘");
        pump({&a, &b}, [&] {
            ChatHistoryQS* h = b.history(AccountRefQS("qq", "10002"));
            return h->conversation("p:10001") != nullptr && h->conversation("p:10001")->messages.size() == 2;
        });
    }
    ChatHistoryQS reloaded("10002", historyFile(root, "10002"));
    QS_CHECK(reloaded.conversation("p:10001") != nullptr);
    QS_CHECK(reloaded.conversation("p:10001")->messages.size() == 2);
    const auto hits = reloaded.search("U盘");
    QS_CHECK(hits.size() == 1 && hits[0].conversationTitle == "丘山");
}

QS_TEST(SameAccountCannotChatFromTwoWindows) {
    const std::string root = freshDir("conflict");
    ChatNodeQS a(root), b(root);
    a.start();
    b.start();
    QS_CHECK(a.addIdentity(AccountRefQS("qq", "10001"), "丘山", historyFile(root, "10001")));
    QS_CHECK(!b.addIdentity(AccountRefQS("qq", "10001"), "丘山", historyFile(root, "10001-b")));
    a.removeIdentity(AccountRefQS("qq", "10001"));
    QS_CHECK(b.addIdentity(AccountRefQS("qq", "10001"), "丘山", historyFile(root, "10001-b")));
}

QS_TEST(ValidatorRejectsStrangers) {
    const std::string root = freshDir("validator");
    ChatNodeQS a(root), b(root);
    a.start();
    b.start();
    b.setValidator([](const AccountRefQS&, const ChatMessageQS& m) { return m.from != "10004"; });
    a.addIdentity(AccountRefQS("qq", "10004"), "江屿", historyFile(root, "10004"));
    b.addIdentity(AccountRefQS("qq", "10002"), "林然", historyFile(root, "10002"));
    a.sendPrivate(AccountRefQS("qq", "10004"), "10002", "林然", "陌生人消息");
    pump({&a, &b}, [] { return false; }, 600);
    QS_CHECK(b.history(AccountRefQS("qq", "10002"))->conversation("p:10004") == nullptr);
}

QS_TEST(ManualConnectExchangesRoster) {
    // 模拟两台电脑：数据目录不同（互相看不到在线公告板），通过手动连接交换在线账号。
    const std::string rootA = freshDir("lan-a"), rootB = freshDir("lan-b");
    ChatNodeQS a(rootA), b(rootB);
    a.start();
    b.start();
    a.addIdentity(AccountRefQS("qq", "10001"), "丘山", historyFile(rootA, "10001"));
    b.addIdentity(AccountRefQS("qq", "10002"), "林然", historyFile(rootB, "10002"));
    QS_CHECK(!a.isOnline(AccountRefQS("qq", "10002")));
    QS_CHECK(a.connectTo("127.0.0.1", b.port()));
    QS_CHECK(pump({&a, &b}, [&] { return a.isOnline(AccountRefQS("qq", "10002")) && b.isOnline(AccountRefQS("qq", "10001")); }));
    b.sendPrivate(AccountRefQS("qq", "10002"), "10001", "丘山", "跨机器消息");
    QS_CHECK(pump({&a, &b}, [&] { return lastIn(a, AccountRefQS("qq", "10001"), "p:10002") != nullptr; }));
}

int main() { return qs::TestHarnessQS::instance().run(); }
