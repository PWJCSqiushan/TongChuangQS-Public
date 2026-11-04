#include "ConsoleContactsQS.h"

namespace qs {

namespace {

void listFriends(ConsoleContextQS& c, const std::vector<FriendViewQS>& list) {
    if (list.empty()) {
        c.io.print("  （空）");
        return;
    }
    for (const auto& f : list) {
        std::string line = "  " + f.account + "  " + f.nickname;
        if (!f.remark.empty()) line += "  备注:" + f.remark;
        if (!f.tag.empty()) line += "  [" + f.tag + "]";
        if (c.svc().supportsChat()) line += c.client.isOnline(c.service, f.account) ? "  ● 在线" : "  ○ 离线";
        c.io.print(line);
    }
}

std::string pickOtherService(ConsoleContextQS& c) {
    std::vector<ServiceBaseQS*> options;
    for (const auto& s : c.client.platform().services()) {
        if (s->id() != c.service && s->friendCapability() != nullptr) options.push_back(s.get());
    }
    for (std::size_t i = 0; i < options.size(); ++i) c.io.print("  " + std::to_string(i + 1) + ". " + options[i]->name());
    const int pick = c.io.readChoice("选择来源服务：", 1, static_cast<int>(options.size()));
    return options[static_cast<std::size_t>(pick - 1)]->id();
}

}  // namespace

void ConsoleContactsQS::run(ConsoleContextQS& c) {
    while (!c.io.eof()) {
        std::lock_guard<std::recursive_mutex> guard(c.lock);
        FriendCapabilityQS* f = c.svc().friendCapability();
        const std::string me = c.me();
        const auto incoming = f->incomingRequests(me);
        c.io.title(c.svc().name() + " · " + f->relationName());
        c.io.print("  1. " + f->relationName() + "列表        2. 查询");
        c.io.print("  3. " + f->addActionName() + "（按" + c.svc().accountLabel() + "）");
        c.io.print("  4. " + std::string(f->requiresApproval() ? "好友申请" : "新粉丝") + "（" + std::to_string(incoming.size()) + "）");
        c.io.print("  5. 修改备注/分组      6. 删除" + f->relationName());
        c.io.print("  7. 从其他服务推荐好友  8. 两个服务的共同好友");
        c.io.print("  0. 返回");
        switch (c.io.readChoice("请选择：", 0, 8)) {
            case 0: return;
            case 1: listFriends(c, f->friendsOf(me)); break;
            case 2: {
                const std::string key = c.io.readLine("关键字（昵称/号码/备注）：");
                std::vector<FriendViewQS> hits;
                for (const auto& v : f->friendsOf(me)) {
                    if (v.account.find(key) != std::string::npos || v.nickname.find(key) != std::string::npos ||
                        v.remark.find(key) != std::string::npos)
                        hits.push_back(v);
                }
                listFriends(c, hits);
                break;
            }
            case 3: {
                const std::string target = c.io.readLine("对方" + c.svc().accountLabel() + "：");
                const std::string note = f->requiresApproval() ? c.io.readLine("验证信息：") : "";
                c.show(f->addFriend(me, target, note));
                break;
            }
            case 4: {
                if (incoming.empty()) { c.io.print("  没有待处理的申请"); break; }
                for (std::size_t i = 0; i < incoming.size(); ++i) {
                    c.io.print("  " + std::to_string(i + 1) + ". " + c.svc().nicknameOf(incoming[i].from) + "（" +
                               incoming[i].from + "）：" + incoming[i].note);
                }
                const int pick = c.io.readChoice("处理第几条（0 返回）：", 0, static_cast<int>(incoming.size()));
                if (pick == 0) break;
                const bool accept = c.io.confirm(f->requiresApproval() ? "同意该申请？" : "回关对方？");
                c.show(f->answerRequest(me, incoming[static_cast<std::size_t>(pick - 1)].id, accept));
                break;
            }
            case 5: {
                const std::string target = c.io.readLine("对方号码：");
                const std::string remark = c.io.readLine("备注（可留空）：");
                const std::string tag = c.io.readLine("分组，如 同学/朋友/家人（可留空）：");
                c.show(f->updateFriend(me, target, remark, tag));
                break;
            }
            case 6: {
                const std::string target = c.io.readLine("要删除的号码：");
                if (c.io.confirm("确定删除？")) c.show(f->removeFriend(me, target));
                break;
            }
            case 7: {
                const std::string source = pickOtherService(c);
                std::string hint;
                const auto list = c.client.platform().recommendFriends(c.service, me, source, hint);
                c.io.print("  " + hint);
                for (std::size_t i = 0; i < list.size(); ++i) {
                    c.io.print("  " + std::to_string(i + 1) + ". " + list[i].nickname + "（" + list[i].account + "）  " + list[i].reason);
                }
                if (list.empty()) break;
                const int pick = c.io.readChoice("添加第几位（0 返回）：", 0, static_cast<int>(list.size()));
                if (pick > 0) {
                    c.show(f->addFriend(me, list[static_cast<std::size_t>(pick - 1)].account,
                                        "我是" + c.svc().nicknameOf(me) + "，从" +
                                            c.client.platform().service(source)->name() + "添加你"));
                }
                break;
            }
            case 8: {
                const std::string other = pickOtherService(c);
                std::string hint;
                const auto list = c.client.platform().commonFriends(c.client.me(c.service), other, hint);
                c.io.print("  " + hint);
                for (const auto& x : list) {
                    c.io.print("  " + x.nickname + "：" + c.svc().name() + " " + x.first.account() + " / " +
                               c.client.platform().service(other)->name() + " " + x.second.account());
                }
                break;
            }
        }
    }
}

QS_REGISTER_CONSOLE_FEATURE(ConsoleContactsQS, "contacts")

}  // namespace qs
