#include "QQGroupPolicyQS.h"

#include "GroupPolicyRegistryQS.h"

namespace qs {

std::vector<std::string> QQGroupPolicyQS::highlights() const {
    return {"任何人都可以申请加入，由群主或管理员审批",
            "群主可任命最多5名管理员，管理员协助审批、移出普通成员",
            "群成员可以发起临时讨论组（子群）",
            "群主、管理员可以发布群公告"};
}

bool QQGroupPolicyQS::canKick(GroupRoleQS actor, GroupRoleQS target) const {
    if (target == GroupRoleQS::Owner) {
        return false;
    }
    if (actor == GroupRoleQS::Owner) {
        return true;
    }
    return actor == GroupRoleQS::Admin && target == GroupRoleQS::Member;
}

QS_REGISTER_GROUP_POLICY(QQGroupPolicyQS)

}  // namespace qs
