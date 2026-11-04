#include "ChatNodeQS.h"
#include "ClientQS.h"
#include "FileLockQS.h"
#include "FrameCodecQS.h"
#include "JsonFileQS.h"
#include "PathQS.h"
#include "PlatformQS.h"
#include "TestHarnessQS.h"

#include <chrono>
#include <fstream>
#include <atomic>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace qs;
namespace {
std::filesystem::path fresh(const char* tag) {
    const auto p = std::filesystem::temp_directory_path() /
        (std::string("qs-delivery-") + tag + "-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(p);
    return p;
}
void pump(ChatNodeQS& a, ChatNodeQS& b, int milliseconds = 500) {
    const auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
    do { a.poll(); b.poll(); a.tick(); b.tick(); std::this_thread::sleep_for(std::chrono::milliseconds(20)); }
    while (std::chrono::steady_clock::now() < until);
}
bool sendFrame(SocketQS& socket, const JsonQS& j) {
    const auto bytes = FrameCodecQS::encode(j.dump());
    return socket.sendAll(bytes.data(), bytes.size());
}
}

QS_TEST(FailedServiceCommitRestoresMemoryAndDisk) {
    const auto root = fresh("rollback");
    PlatformQS p(PathQS::toUtf8(root)); p.start();
    auto* s = p.service("qq");
    const auto before = s->accounts().toJson();
    const auto revision = s->revision();
    std::filesystem::create_directory(root / "qq/revision.json.tmp"); // deterministic write failure, no deletion
    QS_CHECK(!s->updateProfile("10001", "nickname", "must-roll-back"));
    QS_CHECK(s->accounts().toJson() == before);
    QS_CHECK(s->revision() == revision);
    PlatformQS reopened(PathQS::toUtf8(root)); reopened.start();
    QS_CHECK(reopened.service("qq")->accounts().toJson() == before);
    QS_CHECK(reopened.service("qq")->revision() == revision);
}

QS_TEST(StartupDoesNotSeedThroughAnUnavailableLock) {
    const auto root = fresh("lock");
    {
        FileLockQS held(root / "qq/.lock", 0);
        QS_CHECK(held.locked());
        PlatformQS p(PathQS::toUtf8(root));
        QS_CHECK(!p.start());
        QS_CHECK(p.services().empty());
        QS_CHECK(!std::filesystem::exists(root / "qq/accounts.json"));
        QS_CHECK(!std::filesystem::exists(root / "qq/revision.json"));
    }
    PlatformQS recovered(PathQS::toUtf8(root)); recovered.start();
    QS_CHECK(recovered.service("qq")->account("10001") != nullptr);
}

QS_TEST(CorruptSnapshotsArePreservedAndDoNotSeedOrFallBack) {
    const auto root = fresh("corrupt");
    { PlatformQS p(PathQS::toUtf8(root)); QS_CHECK(p.start()); }
    const auto file = root / "qq/revision.json";
    std::ofstream(file, std::ios::trunc) << "{broken";
    PlatformQS rejected(PathQS::toUtf8(root));
    QS_CHECK(!rejected.start());
    std::ifstream in(file); std::string text; std::getline(in, text);
    QS_CHECK(text == "{broken");
}

QS_TEST(LegacySplitFilesMigrateWithoutRemovingOriginals) {
    const auto original = fresh("legacy-export"), imported = fresh("legacy-import");
    PlatformQS p(PathQS::toUtf8(original)); QS_CHECK(p.start());
    const auto snapshot = JsonFileQS::read(original / "qq/revision.json");
    // Export the original array-based layout, then import it into a fresh tree.
    if (!snapshot.contains("files")) return; // original baseline has no snapshot format yet
    for (auto it = snapshot["files"].begin(); it != snapshot["files"].end(); ++it) {
        auto value = it.value();
        if (it.key() == "accounts.json") value = value["accounts"];
        QS_CHECK(JsonFileQS::write(imported / "qq" / it.key(), value));
    }
    QS_CHECK(JsonFileQS::write(imported / "qq/revision.json", JsonQS{{"revision", 1}, {"service", "qq"}}));
    PlatformQS migrated(PathQS::toUtf8(imported)); QS_CHECK(migrated.start());
    auto* qq = migrated.service("qq");
    QS_CHECK(qq->accounts().size() == 8);
    QS_CHECK(qq->updateProfile("10001", "nickname", "migrated"));
    QS_CHECK(JsonFileQS::read(imported / "qq/revision.json").contains("files"));
    QS_CHECK(std::filesystem::exists(imported / "qq/accounts.json"));
    PlatformQS reopened(PathQS::toUtf8(imported)); QS_CHECK(reopened.start());
    QS_CHECK(reopened.service("qq")->nicknameOf("10001") == "migrated");
}

QS_TEST(DeactivatedIdentifiersCannotInheritSurvivingLinks) {
    const auto root = fresh("id-reuse");
    PlatformQS p(PathQS::toUtf8(root)); QS_CHECK(p.start());
    QS_CHECK(p.login("qq", "10001", "123456"));
    QS_CHECK(p.confirmPending("wechat"));
    QS_CHECK(p.service("qq")->deactivate("10001", "123456"));
    p.refreshAll();
    QS_CHECK(!p.session().isLoggedIn("qq"));
    QS_CHECK(p.session().isLoggedIn("wechat"));
    PlatformQS reopened(PathQS::toUtf8(root)); QS_CHECK(reopened.start());
    std::string id;
    QS_CHECK(reopened.service("qq")->registerAccount(FormQS{{"nickname", "new person"}, {"password", "abcdef"},
        {"birthday", "2006-01-01"}, {"location", "test"}}, id));
    QS_CHECK(id != "10001");
    QS_CHECK(reopened.linkedAccountIn(AccountRefQS("qq", id), "wechat").empty());
}

QS_TEST(SameRevisionWithBadMetadataCannotBeOverwritten) {
    const auto root = fresh("metadata");
    PlatformQS p(PathQS::toUtf8(root)); QS_CHECK(p.start());
    const auto file = root / "qq/revision.json";
    auto invalid = JsonFileQS::read(file); invalid["format"] = 99;
    QS_CHECK(JsonFileQS::write(file, invalid));
    p.refreshAll();
    QS_CHECK(!p.service("qq")->storageError().empty());
    QS_CHECK(!p.login("qq", "10001", "123456"));
    QS_CHECK(!p.service("qq")->updateProfile("10001", "nickname", "do not overwrite"));
    QS_CHECK(JsonFileQS::read(file) == invalid);
}

QS_TEST(PartialStartupDoesNotPublishAnyServices) {
    const auto root = fresh("partial-start");
    { PlatformQS seed(PathQS::toUtf8(root)); QS_CHECK(seed.start()); }
    std::ofstream(root / "wechat/revision.json", std::ios::trunc) << "bad";
    PlatformQS p(PathQS::toUtf8(root));
    QS_CHECK(!p.start());
    QS_CHECK(p.services().empty());
}

QS_TEST(UncompiledServiceStorageIsRejectedBeforeAnyDataMutation) {
    const auto root = fresh("absent-service");
    const auto marker = root / "absent-plugin/revision.json";
    // The marker contents are deliberately not JSON: only its presence matters.
    std::filesystem::create_directories(marker.parent_path());
    std::ofstream(marker) << "private data must not be read";
    PlatformQS p(PathQS::toUtf8(root));
    const auto loaded = p.start();
    QS_CHECK(!loaded);
    QS_CHECK(loaded.message().find("absent-plugin") != std::string::npos);
    QS_CHECK(p.services().empty());
    QS_CHECK(!std::filesystem::exists(root / "qq"));
    std::ifstream in(marker); std::string original; std::getline(in, original);
    QS_CHECK(original == "private data must not be read");
}

QS_TEST(NonDirectoryDataRootReportsStartupFailure) {
    const auto file = fresh("file-root") / "ordinary-file";
    std::ofstream(file) << "preserve";
    PlatformQS p(PathQS::toUtf8(file));
    QS_CHECK(!p.start());
    QS_CHECK(p.services().empty());
    std::ifstream in(file); std::string original; std::getline(in, original);
    QS_CHECK(original == "preserve");
}

QS_TEST(LegacyDanglingBindingsReserveMissingIdentifiers) {
    const auto source = fresh("dangling-source"), dest = fresh("dangling-dest");
    PlatformQS seed(PathQS::toUtf8(source)); QS_CHECK(seed.start());
    for (const auto& s : seed.services()) {
        auto snapshot = JsonFileQS::read(source / s->id() / "revision.json");
        for (auto it = snapshot["files"].begin(); it != snapshot["files"].end(); ++it) {
            auto value = it.value();
            if (it.key() == "accounts.json") {
                value = value["accounts"];
                if (s->id() == "wechat") value[0]["links"].push_back(AccountRefQS("qq", "10009").toJson());
            }
            QS_CHECK(JsonFileQS::write(dest / s->id() / it.key(), value));
        }
        QS_CHECK(JsonFileQS::write(dest / s->id() / "revision.json", JsonQS{{"revision", 1}, {"service", s->id()}}));
    }
    PlatformQS migrated(PathQS::toUtf8(dest)); QS_CHECK(migrated.start());
    std::string id;
    QS_CHECK(migrated.service("qq")->registerAccount(FormQS{{"nickname", "new person"}, {"password", "abcdef"},
        {"birthday", "2006-01-01"}, {"location", "test"}}, id));
    QS_CHECK(id != "10009");
    PlatformQS reopened(PathQS::toUtf8(dest)); QS_CHECK(reopened.start());
    QS_CHECK(reopened.service("qq")->accounts().isRetired("10009"));
}

QS_TEST(ConcurrentNumericRegistrationsKeepBothUpdates) {
    const auto root = fresh("parallel");
    PlatformQS a(PathQS::toUtf8(root)), b(PathQS::toUtf8(root)); QS_CHECK(a.start()); QS_CHECK(b.start());
    std::atomic<bool> go{false}; bool okA = false, okB = false; std::string idA, idB;
    const FormQS form{{"nickname", "parallel"}, {"password", "abcdef"}, {"birthday", "2006-01-01"}, {"location", "test"}};
    std::thread t1([&] { while (!go) std::this_thread::yield(); okA = a.service("qq")->registerAccount(form, idA).isOk(); });
    std::thread t2([&] { while (!go) std::this_thread::yield(); okB = b.service("qq")->registerAccount(form, idB).isOk(); });
    go = true; t1.join(); t2.join();
    QS_CHECK(okA && okB); QS_CHECK(!idA.empty() && idA != idB);
    PlatformQS reopened(PathQS::toUtf8(root)); QS_CHECK(reopened.start());
    QS_CHECK(reopened.service("qq")->accounts().size() == 10);
}

QS_TEST(SimultaneousIdentityClaimsHaveExactlyOneOwner) {
    const auto root = fresh("identity-race");
    ChatNodeQS a(PathQS::toUtf8(root)), b(PathQS::toUtf8(root)); a.start(); b.start();
    const AccountRefQS identity("qq", "same");
    std::atomic<bool> go{false}; bool okA = false, okB = false;
    std::thread t1([&] { while (!go) std::this_thread::yield(); okA = a.addIdentity(identity, "same", root / "a.json").isOk(); });
    std::thread t2([&] { while (!go) std::this_thread::yield(); okB = b.addIdentity(identity, "same", root / "b.json").isOk(); });
    go = true; t1.join(); t2.join();
    QS_CHECK(okA != okB);
    if (okA) { a.removeIdentity(identity); QS_CHECK(b.addIdentity(identity, "same", root / "b.json")); }
    else { b.removeIdentity(identity); QS_CHECK(a.addIdentity(identity, "same", root / "a.json")); }
}

QS_TEST(ConfirmedLinkedLoginCannotBypassChatExclusivity) {
    const auto root = fresh("confirm-login");
    ClientQS a(PathQS::toUtf8(root)), b(PathQS::toUtf8(root)); QS_CHECK(a.start()); QS_CHECK(b.start());
    QS_CHECK(a.login("qq", "10001", "123456")); QS_CHECK(a.confirmPending("wechat"));
    QS_CHECK(b.login("weibo", "10001", "123456"));
    QS_CHECK(!b.confirmPending("wechat"));
    QS_CHECK(!b.platform().session().isLoggedIn("wechat"));
    QS_CHECK(a.chatReady("wechat"));
}

QS_TEST(GroupPermissionsAndPolicyRoundTripSurviveRestart) {
    const auto root = fresh("policy-permissions");
    PlatformQS p(PathQS::toUtf8(root)); QS_CHECK(p.start());
    auto* g = p.service("qq")->groupCapability();
    QS_CHECK(!g->kick("10003", "1001", "10002"));
    QS_CHECK(!g->kick("10002", "1001", "10001"));
    QS_CHECK(!g->setAdmin("10003", "1001", "10005", true));
    QS_CHECK(!g->setAnnouncement("10003", "1001", "denied"));
    QS_CHECK(!g->switchPolicy("10004", "1001", "wechat"));
    QS_CHECK(g->createSubgroup("10003", "1001", "member discussion", {"10002"}));
    GroupQS before; QS_CHECK(g->findGroup("1001", before));
    QS_CHECK(g->switchPolicy("10001", "1001", "wechat"));
    QS_CHECK(!g->kick("10002", "1001", "10003"));
    QS_CHECK(!g->requestJoin("10004", "1001", "denied"));
    PlatformQS reopened(PathQS::toUtf8(root)); QS_CHECK(reopened.start());
    auto* restored = reopened.service("qq")->groupCapability();
    QS_CHECK(restored->switchPolicy("10001", "1001", "qq"));
    GroupQS after; QS_CHECK(restored->findGroup("1001", after));
    QS_CHECK(after.toJson() == before.toJson());
    std::filesystem::create_directory(root / "qq/revision.json.tmp");
    QS_CHECK(!restored->kick("10001", "1001", "10003"));
    GroupQS rolledBack; restored->findGroup("1001", rolledBack);
    QS_CHECK(rolledBack.toJson() == before.toJson());
}

QS_TEST(MissingActorsCannotCreateFriendOrGroupState) {
    const auto root = fresh("missing-actor");
    PlatformQS p(PathQS::toUtf8(root)); QS_CHECK(p.start());
    for (const auto& s : p.services()) {
        const auto id = s->accounts().all().front()->id();
        if (auto* f = s->friendCapability()) QS_CHECK(!f->addFriend("missing-user", id, "bad actor"));
        if (auto* g = s->groupCapability()) {
            std::string group;
            QS_CHECK(!g->createGroup("missing-user", "invalid owner", group));
            QS_CHECK(!g->requestJoin("missing-user", "1001", "bad actor"));
        }
    }
}

QS_TEST(JsonReplacementFailurePreservesTheCommittedFile) {
    const auto root = fresh("replace");
    const auto file = root / "data.json";
    QS_CHECK(JsonFileQS::write(file, JsonQS{{"value", "old"}}));
#ifdef _WIN32
    HANDLE h = CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    QS_CHECK(h != INVALID_HANDLE_VALUE);
    QS_CHECK(!JsonFileQS::write(file, JsonQS{{"value", "new"}}));
    CloseHandle(h);
    QS_CHECK(JsonFileQS::read(file).value("value", std::string()) == "old");
#endif
}

QS_TEST(LocalRejectionKeepsTheMessagePending) {
    const auto root = fresh("local-reject");
    ChatNodeQS n(PathQS::toUtf8(root)); n.start();
    const AccountRefQS a("qq", "a"), b("qq", "b");
    n.addIdentity(a, "a", root / "a.json"); n.addIdentity(b, "b", root / "b.json");
    n.setValidator([](const AccountRefQS&, const ChatMessageQS&) { return false; });
    const auto m = n.sendPrivate(a, "b", "b", "reject me");
    QS_CHECK(m.pendingTo == std::vector<std::string>{"b"});
    QS_CHECK(n.history(b)->find(m.id) == nullptr);
    n.setValidator({}); n.tick();
    QS_CHECK(n.history(b)->find(m.id) != nullptr);
    QS_CHECK(n.history(a)->find(m.id)->status == MessageStatusQS::Delivered);
}

QS_TEST(SenderCannotSendAnUnsavedMessage) {
    const auto root = fresh("sender-disk");
    ChatNodeQS a(PathQS::toUtf8(root)), b(PathQS::toUtf8(root)); a.start(); b.start();
    const AccountRefQS r1("qq", "a"), r2("qq", "b");
    a.addIdentity(r1, "a", root / "a.json"); b.addIdentity(r2, "b", root / "b.json");
    std::filesystem::create_directory(root / "a.json.tmp");
    const auto sent = a.sendPrivate(r1, "b", "b", "must not leave this process");
    pump(a, b);
    QS_CHECK(sent.id.empty());
    QS_CHECK(b.history(r2)->search("must not leave").empty());
}

QS_TEST(ReceiverDoesNotAcknowledgeUntilDurablySaved) {
    const auto root = fresh("receiver-disk");
    ChatNodeQS a(PathQS::toUtf8(root)), b(PathQS::toUtf8(root)); a.start(); b.start();
    const AccountRefQS r1("qq", "a"), r2("qq", "b");
    a.addIdentity(r1, "a", root / "a.json"); b.addIdentity(r2, "b", root / "b.json");
    std::filesystem::create_directory(root / "b.json.tmp");
    const auto sent = a.sendPrivate(r1, "b", "b", "retry after disk recovers");
    pump(a, b);
    const auto* m = a.history(r1)->find(sent.id);
    QS_CHECK(m != nullptr && m->status != MessageStatusQS::Delivered && !m->pendingTo.empty());
    // Keep the obstruction as evidence; moving it restores writes without deleting a file.
    std::filesystem::rename(root / "b.json.tmp", root / "saved-obstruction");
    pump(a, b, 1600);
    QS_CHECK(a.history(r1)->find(sent.id)->status == MessageStatusQS::Delivered);
    ChatHistoryQS disk("b", root / "b.json");
    QS_CHECK(disk.search("retry after disk recovers").size() == 1);
}

QS_TEST(AcknowledgementStateWaitsForSenderStorageRecovery) {
    const auto root = fresh("ack-disk");
    ChatNodeQS a(PathQS::toUtf8(root)), b(PathQS::toUtf8(root)); a.start(); b.start();
    const AccountRefQS r1("qq", "a"), r2("qq", "b");
    a.addIdentity(r1, "a", root / "a.json"); b.addIdentity(r2, "b", root / "b.json");
    const auto sent = a.sendPrivate(r1, "b", "b", "ack must commit");
    std::filesystem::create_directory(root / "a.json.tmp");
    pump(a, b, 300);
    QS_CHECK(a.history(r1)->find(sent.id)->status != MessageStatusQS::Delivered);
    QS_CHECK(!a.history(r1)->find(sent.id)->pendingTo.empty());
    std::filesystem::rename(root / "a.json.tmp", root / "saved-obstruction");
    pump(a, b, 600);
    QS_CHECK(a.history(r1)->find(sent.id)->status == MessageStatusQS::Delivered);
    QS_CHECK(b.history(r2)->search("ack must commit").size() == 1);
}

QS_TEST(ForgedReceiptAndMalformedJsonDoNotChangeDelivery) {
    const auto root = fresh("forged-ack");
    ChatNodeQS a(PathQS::toUtf8(root)), unused(PathQS::toUtf8(root)); a.start(); unused.start();
    const AccountRefQS me("qq", "a"); a.addIdentity(me, "a", root / "a.json");
    const auto sent = a.sendPrivate(me, "b", "b", "pending");
    std::string error;
    auto socket = SocketQS::connectTo("127.0.0.1", a.port(), 1000, error);
    QS_CHECK(socket.valid());
    QS_CHECK(sendFrame(socket, JsonQS{{"type", "ack"}, {"service", "qq"}, {"to", "a"}, {"from", "intruder"}, {"id", sent.id}}));
    pump(a, unused, 200);
    QS_CHECK(a.history(me)->find(sent.id)->ackedBy.empty());
    QS_CHECK(a.history(me)->find(sent.id)->status == MessageStatusQS::Pending);
    // Valid JSON with wrong field types used to escape poll() as an exception.
    QS_CHECK(sendFrame(socket, JsonQS{{"type", 42}}));
    bool survived = true;
    try { pump(a, unused, 150); } catch (...) { survived = false; }
    QS_CHECK(survived);
}

QS_TEST(CodecHandlesSplitsCoalescingAndInvalidFrames) {
    const auto one = FrameCodecQS::encode("中文");
    const auto two = FrameCodecQS::encode("second");
    for (std::size_t split = 0; split < one.size(); ++split) {
        std::vector<std::uint8_t> buffer(one.begin(), one.begin() + split);
        const auto before = buffer; std::string text = "unchanged";
        QS_CHECK(!FrameCodecQS::tryDecode(buffer, text) && buffer == before && text == "unchanged");
        buffer.insert(buffer.end(), one.begin() + split, one.end());
        buffer.insert(buffer.end(), two.begin(), two.end());
        QS_CHECK(FrameCodecQS::tryDecode(buffer, text) && text == "中文");
        QS_CHECK(FrameCodecQS::tryDecode(buffer, text) && text == "second" && buffer.empty());
    }
    for (auto buffer : {std::vector<std::uint8_t>{0,0,0,0}, std::vector<std::uint8_t>{0,4,0,1},
                       std::vector<std::uint8_t>{0,0,0,2,0xC0,0x80}}) {
        bool rejected = false; std::string text;
        try { FrameCodecQS::tryDecode(buffer, text); } catch (...) { rejected = true; }
        QS_CHECK(rejected);
    }
}

QS_TEST(UnacknowledgedMessagesRetryAfterSenderRestartWithoutDuplicates) {
    const auto root = fresh("restart");
    const AccountRefQS me("qq", "a"), peer("qq", "b");
    ChatNodeQS receiver(PathQS::toUtf8(root)); receiver.start();
    receiver.addIdentity(peer, "b", root / "b.json");
    std::string id;
    {
        ChatNodeQS sender(PathQS::toUtf8(root)); sender.start();
        sender.addIdentity(me, "a", root / "a.json");
        id = sender.sendPrivate(me, "b", "b", "survive lost acknowledgement").id;
        // Process incoming on the receiver only. The sender never consumes its ACK.
        for (int i = 0; i < 20; ++i) { receiver.poll(); std::this_thread::sleep_for(std::chrono::milliseconds(20)); }
        QS_CHECK(receiver.history(peer)->find(id) != nullptr);
    }
    ChatNodeQS restarted(PathQS::toUtf8(root)); restarted.start();
    restarted.addIdentity(me, "a", root / "a.json");
    pump(restarted, receiver, 1200);
    const auto* sent = restarted.history(me)->find(id);
    QS_CHECK(sent && sent->status == MessageStatusQS::Delivered && sent->pendingTo.empty());
    QS_CHECK(receiver.history(peer)->search("survive lost acknowledgement").size() == 1);
}

int main() { return TestHarnessQS::instance().run(); }
