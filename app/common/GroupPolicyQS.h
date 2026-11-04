#pragma once

#include "GroupMemberQS.h"

#include <memory>
#include <string>
#include <vector>

namespace qs {

/** 加入群的方式。 */
enum class JoinModeQS {
    Application,  // 申请 → 群主/管理员审批（QQ 群）
    InviteOnly,   // 只能由群成员邀请（微信群）
    Open          // 满足条件即可直接加入（微博粉丝群：关注了群主即可加入）
};

/**
 * 群管理模式（策略模式的抽象基类）——本项目"多态"最核心的落点之一。
 *
 * GroupQS 只保存成员、申请、讨论组等"数据"，所有"规则"都委托给它持有的
 * GroupPolicyQS 指针，通过虚函数在运行时决定：能否申请、谁能审批、谁能踢人、
 * 是否有管理员、能否建讨论组……
 * 把同一个群的策略对象换成另一个子类，就实现了"在群成员数据不受伤害的前提下，
 * 动态变换为其他类型群的管理特色"。
 */
class GroupPolicyQS {
public:
    virtual ~GroupPolicyQS() = default;

    /** 唯一标识，如 "qq"、"wechat"、"weibo"，用于存档。 */
    virtual std::string kind() const = 0;
    virtual std::string displayName() const = 0;
    /** 该模式的特色说明（界面"群特色"面板逐条展示）。 */
    virtual std::vector<std::string> highlights() const = 0;

    virtual JoinModeQS joinMode() const = 0;
    virtual bool supportsAdmins() const = 0;
    virtual int maxAdmins() const { return supportsAdmins() ? 5 : 0; }
    virtual bool supportsSubgroups() const = 0;

    virtual bool canApprove(GroupRoleQS actor) const = 0;
    virtual bool canInvite(GroupRoleQS actor) const = 0;
    virtual bool canKick(GroupRoleQS actor, GroupRoleQS target) const = 0;
    virtual bool canSetAdmin(GroupRoleQS actor) const { return supportsAdmins() && actor == GroupRoleQS::Owner; }
    virtual bool canCreateSubgroup(GroupRoleQS actor) const = 0;
    virtual bool canEditAnnouncement(GroupRoleQS actor) const = 0;

    /**
     * 存档里的角色在当前模式下实际生效的角色。
     * 例如切换为微信模式后，原管理员的身份仍然保存，但按普通成员对待；
     * 切回 QQ 模式后自动恢复——这就是"成员数据不受伤害"。
     */
    virtual GroupRoleQS effectiveRole(GroupRoleQS stored) const {
        return (stored == GroupRoleQS::Admin && !supportsAdmins()) ? GroupRoleQS::Member : stored;
    }

    /** 拒绝申请加入时给用户的说明。 */
    virtual std::string joinHint() const = 0;

    virtual std::unique_ptr<GroupPolicyQS> clone() const = 0;
};

}  // namespace qs
