#include "ConsoleGroupsQS.h"

#include "GroupPolicyRegistryQS.h"

namespace qs {

namespace {

void printGroup(ConsoleContextQS& c, const GroupQS& g) {
    c.io.print("  【" + g.id() + "】" + g.name() + "  · " + g.policy().displayName() + " · " +
               std::to_string(g.members().size()) + "人");
    if (!g.announcement().empty()) c.io.print("  公告：" + g.announcement());
    for (const auto& h : g.policy().highlights()) c.io.print("    - " + h);
    c.io.print("  成员：");
    for (const auto& m : g.members()) {
        GroupRoleQS effective = m.role();
        g.effectiveRole(m.account(), effective);
        std::string line = "    " + m.account() + " " + c.svc().nicknameOf(m.account()) + "  " + roleName(effective);
        if (effective != m.role()) line += "（保留身份：" + roleName(m.role()) + "）";
        c.io.print(line);
    }
    if (!g.subgroups().empty()) {
        c.io.print(std::string("  讨论组") + (g.policy().supportsSubgroups() ? "：" : "（当前模式下休眠）："));
        for (const auto& s : g.subgroups()) c.io.print("    " + s.name + "（" + std::to_string(s.members.size()) + "人）");
    }
    if (!g.applications().empty()) c.io.print("  待审批申请：" + std::to_string(g.applications().size()) + " 条");
}

}  // namespace

void ConsoleGroupsQS::run(ConsoleContextQS& c) {
    while (!c.io.eof()) {
        std::lock_guard<std::recursive_mutex> guard(c.lock);
        GroupCapabilityQS* g = c.svc().groupCapability();
        const std::string me = c.me();
        c.io.title(c.svc().name() + " · 群（待办 " + std::to_string(g->pendingCount(me)) + "）");
        c.io.print("  1. 我的群          2. 群目录（全部群）   3. 查看群详情与成员");
        c.io.print("  4. 申请/加入群     5. 处理入群申请       6. 处理收到的邀请");
        c.io.print("  7. 邀请好友入群    8. 退出群            9. 移出成员");
        c.io.print("  10. 设置/取消管理员 11. 转让群主         12. 创建讨论组");
        c.io.print("  13. 修改群公告      14. 切换群管理模式    15. 新建群");
        c.io.print("  0. 返回");
        const int pick = c.io.readChoice("请选择：", 0, 15);
        if (pick == 0) return;
        if (pick == 1 || pick == 2) {
            for (const auto& x : pick == 1 ? g->groupsOf(me) : g->directory()) {
                c.io.print("  " + x.id() + "  " + x.name() + "  [" + x.policy().displayName() + "] " +
                           std::to_string(x.members().size()) + "人" + (x.hasMember(me) ? "  （已加入）" : ""));
            }
            continue;
        }
        if (pick == 15) {
            std::string id;
            c.show(g->createGroup(me, c.io.readLine("群名称："), id));
            continue;
        }
        const std::string groupId = c.io.readLine("群号：");
        GroupQS group;
        if (!g->findGroup(groupId, group)) {
            c.io.warn("群 " + groupId + " 不存在");
            continue;
        }
        switch (pick) {
            case 3: printGroup(c, group); break;
            case 4: c.show(g->requestJoin(me, groupId, c.io.readLine("申请理由（可留空）："))); break;
            case 5: {
                if (group.applications().empty()) { c.io.print("  没有待审批的申请"); break; }
                for (const auto& a : group.applications()) {
                    c.io.print("  " + a.account + " " + c.svc().nicknameOf(a.account) + "：" + a.note);
                }
                const std::string who = c.io.readLine("处理谁的申请（号码）：");
                c.show(g->reviewApplication(me, groupId, who, c.io.confirm("同意？")));
                break;
            }
            case 6: c.show(g->answerInvitation(me, groupId, c.io.confirm("接受邀请加入「" + group.name() + "」？"))); break;
            case 7: c.show(g->invite(me, groupId, c.io.readLine("被邀请人号码："))); break;
            case 8: if (c.io.confirm("确定退出该群？")) c.show(g->leave(me, groupId)); break;
            case 9: c.show(g->kick(me, groupId, c.io.readLine("要移出的成员号码："))); break;
            case 10: {
                const std::string who = c.io.readLine("成员号码：");
                c.show(g->setAdmin(me, groupId, who, c.io.confirm("设为管理员？（n 为取消管理员）")));
                break;
            }
            case 11: c.show(g->transferOwner(me, groupId, c.io.readLine("新群主号码："))); break;
            case 12: {
                const std::string name = c.io.readLine("讨论组名称：");
                std::vector<std::string> members;
                while (true) {
                    const std::string m = c.io.readLine("添加成员号码（直接回车结束）：");
                    if (m.empty()) break;
                    members.push_back(m);
                }
                c.show(g->createSubgroup(me, groupId, name, members));
                break;
            }
            case 13: c.show(g->setAnnouncement(me, groupId, c.io.readLine("新公告："))); break;
            case 14: {
                c.io.print("  当前：" + group.policy().displayName());
                const auto kinds = GroupPolicyRegistryQS::instance().kinds();
                for (std::size_t i = 0; i < kinds.size(); ++i) {
                    c.io.print("  " + std::to_string(i + 1) + ". " + GroupPolicyRegistryQS::instance().create(kinds[i])->displayName());
                }
                const int k = c.io.readChoice("切换为：", 1, static_cast<int>(kinds.size()));
                c.show(g->switchPolicy(me, groupId, kinds[static_cast<std::size_t>(k - 1)]));
                break;
            }
            default: break;
        }
    }
}

QS_REGISTER_CONSOLE_FEATURE(ConsoleGroupsQS, "groups")

}  // namespace qs
