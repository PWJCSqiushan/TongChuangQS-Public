#include "WeiboGroupPolicyQS.h"

#include "GroupPolicyRegistryQS.h"

namespace qs {

std::vector<std::string> WeiboGroupPolicyQS::highlights() const {
    return {"关注了群主的用户可以直接加入，无需审批",
            "群主可任命最多3名管理员",
            "群主、管理员可以邀请成员、移出普通成员",
            "不支持临时讨论组"};
}

QS_REGISTER_GROUP_POLICY(WeiboGroupPolicyQS)

}  // namespace qs
