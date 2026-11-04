#include "ConsoleAppQS.h"

#include "DateQS.h"

#include <chrono>

namespace qs {

ConsoleAppQS::ConsoleAppQS(const std::string& dataRootUtf8)
    : client_(dataRootUtf8), context_{client_, io_, lock_, "", ""} {}

ConsoleAppQS::~ConsoleAppQS() {
    pumping_ = false;
    if (pump_.joinable()) pump_.join();
}

void ConsoleAppQS::pumpLoop() {
    while (pumping_) {
        {
            std::lock_guard<std::recursive_mutex> guard(lock_);
            bool changed = false;
            for (const auto& e : client_.pump(changed)) {
                if (e.kind != ChatEventQS::KindQS::MessageReceived) continue;
                ChatHistoryQS* h = client_.history(e.owner.service());
                if (h == nullptr) continue;
                ConversationQS* conv = h->conversation(e.conversation);
                const ChatMessageQS* m = conv == nullptr ? nullptr : &conv->messages.back();
                if (m == nullptr) continue;
                if (context_.service == e.owner.service() && context_.activeConversation == e.conversation) {
                    io_.print("  [" + DateQS::clockText(m->time) + "] " + m->fromNick + "：" + m->text);
                    h->markRead(e.conversation);
                } else {
                    io_.print("  🔔 新消息 · " + (m->isGroup() ? "[" + m->groupName + "] " : "") + m->fromNick + "：" + m->text);
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

std::string ConsoleAppQS::badge(const std::string& service) const {
    const LoginSessionQS& s = client_.platform().session();
    if (s.isLoggedIn(service)) return "[已登录 " + s.accountOf(service) + "]";
    if (s.isPending(service)) return "[已开通，可一键进入 " + s.accountOf(service) + "]";
    return "";
}

int ConsoleAppQS::run() {
    const ResultQS started = client_.start();
    io_.print("同窗 · 立体社交平台（控制台版）  数据目录：" + client_.platform().dataRoot());
    if (!started) { io_.warn(started.message()); return 1; }
    pumping_ = true;
    pump_ = std::thread(&ConsoleAppQS::pumpLoop, this);
    while (!io_.eof()) {
        const auto& services = client_.platform().services();
        const int logPick = services.size() < 8 ? 8 : static_cast<int>(services.size()) + 1;
        const int userPick = logPick + 1;
        {
            std::lock_guard<std::recursive_mutex> guard(lock_);
            io_.title("请选择服务");
            for (std::size_t i = 0; i < services.size(); ++i) {
                io_.print("  " + std::to_string(i + 1) + ". " + services[i]->name() + "  —  " + services[i]->slogan() + "  " +
                          badge(services[i]->id()));
            }
            io_.print("  " + std::to_string(logPick) + ". 平台日志（服务之间的通信记录）");
            io_.print("  " + std::to_string(userPick) + ". 切换用户（退出全部服务）");
            io_.print("  0. 退出程序");
        }
        const int pick = io_.readChoice("请选择：", 0, userPick);
        if (pick == 0) break;
        if (pick == logPick) { showBusLog(); continue; }
        if (pick == userPick) {
            std::lock_guard<std::recursive_mutex> guard(lock_);
            client_.logoutAll();
            io_.ok("已退出全部服务");
            continue;
        }
        if (pick > static_cast<int>(services.size())) continue;
        serviceEntry(services[static_cast<std::size_t>(pick - 1)]->id());
        if (context_.service == "__quit__") break;
    }
    pumping_ = false;
    if (pump_.joinable()) pump_.join();
    io_.print("再见！数据已保存。");
    return 0;
}

void ConsoleAppQS::serviceEntry(const std::string& service) {
    ServiceBaseQS* s = client_.platform().service(service);
    while (!io_.eof()) {
        if (client_.platform().session().isLoggedIn(service)) {
            if (serviceHome(service)) context_.service = "__quit__";
            return;
        }
        const bool pending = client_.platform().session().isPending(service);
        io_.title(s->name() + " · " + s->slogan());
        io_.print("  1. 登录");
        io_.print("  2. 注册" + s->name() + "账号（" + s->idRuleText() + "）");
        if (pending) io_.print("  3. 以 " + client_.platform().session().accountOf(service) + " 一键进入（其他服务已登录，确认即可）");
        io_.print("  0. 返回服务选择");
        const int pick = io_.readChoice("请选择：", 0, pending ? 3 : 2);
        if (pick == 0) return;
        if (pick == 1) loginFlow(service);
        if (pick == 2) registerFlow(service);
        if (pick == 3) {
            std::lock_guard<std::recursive_mutex> guard(lock_);
            context_.show(client_.confirmPending(service));
        }
    }
}

bool ConsoleAppQS::loginFlow(const std::string& service) {
    ServiceBaseQS* s = client_.platform().service(service);
    const std::string account = io_.readLine(s->accountLabel() + "：");
    const std::string password = io_.readLine("密码（演示账号均为123456）：");
    std::lock_guard<std::recursive_mutex> guard(lock_);
    const ResultQS r = client_.login(service, account, password);
    context_.show(r);
    if (r) {
        for (const auto& other : client_.platform().services()) {
            if (client_.platform().session().isPending(other->id())) {
                io_.print("  · 发现你已开通" + other->name() + "（" + client_.platform().session().accountOf(other->id()) +
                          "），在服务选择页确认即可直接进入");
            }
        }
    }
    return r.isOk();
}

bool ConsoleAppQS::registerFlow(const std::string& service) {
    ServiceBaseQS* s = client_.platform().service(service);
    FormQS form;
    io_.print("  " + s->idRuleText());
    for (const auto& field : s->registrationFields()) {
        form[field.key] = io_.readLine("  " + field.label + (field.optional ? "（可留空）" : "") + "  " + field.placeholder + "：");
    }
    std::lock_guard<std::recursive_mutex> guard(lock_);
    std::string id;
    const ResultQS r = s->registerAccount(form, id);
    context_.show(r);
    if (!r) return false;
    // 联动开通：由平台把请求发给其他服务，各服务自行注册、分别保存
    std::vector<std::string> chosen;
    for (const std::string& target : client_.platform().linkedRegistrationTargets(service)) {
        ServiceBaseQS* t = client_.platform().service(target);
        if (io_.confirm("  是否" + t->linkedRegistrationHint() + "？")) chosen.push_back(target);
    }
    for (const auto& result : client_.platform().registerLinked(AccountRefQS(service, id), form["password"], chosen)) {
        context_.show(result.second);
    }
    context_.show(client_.login(service, id, form["password"]));
    return true;
}

bool ConsoleAppQS::serviceHome(const std::string& service) {
    ServiceBaseQS* s = client_.platform().service(service);
    while (!io_.eof()) {
        if (!client_.platform().session().isLoggedIn(service)) return false;
        const auto features = s->features();
        const int backPick = features.size() < 8 ? 8 : static_cast<int>(features.size()) + 1;
        const int logoutPick = backPick + 1;
        {
            std::lock_guard<std::recursive_mutex> guard(lock_);
            context_.service = service;
            const std::string me = client_.accountOf(service);
            int unread = 0;
            if (ChatHistoryQS* h = client_.history(service)) unread = h->unreadTotal();
            io_.title(s->name() + " · " + s->nicknameOf(me) + "（" + me + "）");
            for (std::size_t i = 0; i < features.size(); ++i) {
                std::string line = "  " + std::to_string(i + 1) + ". " + features[i].title;
                if (features[i].kind == "chats" && unread > 0) line += "（" + std::to_string(unread) + "条未读）";
                io_.print(line);
            }
            io_.print("  " + std::to_string(backPick) + ". 返回服务选择（保持登录）");
            io_.print("  " + std::to_string(logoutPick) + ". 退出" + s->name() + "登录");
            io_.print("  0. 退出程序");
        }
        const int pick = io_.readChoice("请选择：", 0, logoutPick);
        if (pick == 0) return true;
        if (pick == backPick) return false;
        if (pick == logoutPick) {
            std::lock_guard<std::recursive_mutex> guard(lock_);
            client_.logout(service);
            io_.ok("已退出" + s->name() + "，其他服务保持登录");
            return false;
        }
        if (pick > static_cast<int>(features.size())) continue;
        auto feature = ConsoleFeatureRegistryQS::instance().create(features[static_cast<std::size_t>(pick - 1)].kind);
        if (!feature) {
            io_.warn("控制台暂不支持该功能，请使用图形界面");
            continue;
        }
        feature->run(context_);
    }
    return true;
}

void ConsoleAppQS::showBusLog() {
    std::lock_guard<std::recursive_mutex> guard(lock_);
    io_.title("平台日志：服务之间只通过总线消息协作");
    const auto& log = client_.platform().busLog();
    const std::size_t start = log.size() > 30 ? log.size() - 30 : 0;
    for (std::size_t i = start; i < log.size(); ++i) {
        const auto& e = log[i];
        io_.print("  " + DateQS::clockText(e.time) + "  " + e.from + " → " + e.to + "  " + e.kind + "  " +
                  (e.ok ? "成功" : "失败") + (e.message.empty() ? "" : "：" + e.message));
    }
    if (log.empty()) io_.print("  暂无记录（登录、注册、推荐好友时会产生服务间请求）");
}

}  // namespace qs
