#include "ConsoleProfileQS.h"

namespace qs {

void ConsoleProfileQS::run(ConsoleContextQS& c) {
    while (!c.io.eof()) {
        std::lock_guard<std::recursive_mutex> guard(c.lock);
        ServiceBaseQS& s = c.svc();
        const ServiceAccountQS* a = s.account(c.me());
        if (a == nullptr) return;
        const UserProfileQS& p = a->profile();
        c.io.title(s.name() + " · 我");
        c.io.print("  " + s.accountLabel() + "：" + a->id() + "    昵称：" + p.nickname());
        c.io.print("  出生日期：" + p.birthday() + "    所在地：" + p.location());
        c.io.print("  注册日期：" + p.registeredOn() + "    T龄：" + std::to_string(p.tAge()) + " 年");
        if (!p.signature().empty()) c.io.print("  签名：" + p.signature());
        for (const auto& link : c.client.platform().linkedAccounts(c.client.me(c.service))) {
            ServiceBaseQS* other = c.client.platform().service(link.service());
            c.io.print("  关联：" + (other ? other->name() : link.service()) + " " + link.account());
        }
        c.io.print("  1. 修改资料  2. 修改密码  3. 绑定其他服务账号  4. 注销本服务账号  0. 返回");
        const int pick = c.io.readChoice("请选择：", 0, 4);
        if (pick == 0) return;
        if (pick == 1) {
            c.io.print("  1.昵称 2.出生日期 3.所在地 4.个性签名");
            const char* fields[] = {"nickname", "birthday", "location", "signature"};
            const int f = c.io.readChoice("修改哪一项：", 1, 4);
            c.show(s.updateProfile(c.me(), fields[f - 1], c.io.readLine("新内容：")));
        } else if (pick == 2) {
            const std::string oldPassword = c.io.readLine("原密码：");
            c.show(s.changePassword(c.me(), oldPassword, c.io.readLine("新密码：")));
        } else if (pick == 3) {
            const auto bindable = s.bindableServices();
            if (bindable.empty()) {
                c.io.warn(s.name() + "不支持绑定其他服务账号");
                continue;
            }
            for (std::size_t i = 0; i < bindable.size(); ++i) {
                c.io.print("  " + std::to_string(i + 1) + ". " + c.client.platform().service(bindable[i])->name());
            }
            const int b = c.io.readChoice("绑定哪个服务：", 1, static_cast<int>(bindable.size()));
            const std::string target = bindable[static_cast<std::size_t>(b - 1)];
            const std::string account = c.io.readLine("对方服务账号：");
            const std::string password = c.io.readLine("对方服务密码（由对方服务核验）：");
            c.show(s.bindAccount(c.me(), AccountRefQS(target, account), password));
        } else if (pick == 4) {
            c.io.warn("注销后本服务的好友、群、聊天记录将不可用，其他服务的账号不受影响");
            const std::string password = c.io.readLine("输入密码确认注销：");
            const std::string me = c.me();
            const ResultQS r = s.deactivate(me, password);
            c.show(r);
            if (r) {
                c.client.logout(c.service);
                return;
            }
        }
    }
}

QS_REGISTER_CONSOLE_FEATURE(ConsoleProfileQS, "me")

}  // namespace qs
