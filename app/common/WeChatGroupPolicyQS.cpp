#include "WeChatGroupPolicyQS.h"

#include "GroupPolicyRegistryQS.h"

namespace qs {

std::vector<std::string> WeChatGroupPolicyQS::highlights() const {
    return {"不能主动申请，只能由群成员邀请（推荐）加入",
            "仅群主为特权账号：只有群主能移出成员、修改群公告",
            "没有管理员制度（原管理员身份保留，但按普通成员对待）",
            "不支持临时讨论组（原有讨论组保留为休眠状态）"};
}

QS_REGISTER_GROUP_POLICY(WeChatGroupPolicyQS)

}  // namespace qs
