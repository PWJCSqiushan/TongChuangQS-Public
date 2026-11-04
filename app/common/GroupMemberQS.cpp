#include "GroupMemberQS.h"

namespace qs {

std::string roleName(GroupRoleQS role) {
    switch (role) {
        case GroupRoleQS::Owner: return "群主";
        case GroupRoleQS::Admin: return "管理员";
        case GroupRoleQS::Member: return "成员";
    }
    return "成员";
}

GroupRoleQS roleFromText(const std::string& text) {
    if (text == "owner") return GroupRoleQS::Owner;
    if (text == "admin") return GroupRoleQS::Admin;
    return GroupRoleQS::Member;
}

namespace {
std::string roleKey(GroupRoleQS role) {
    switch (role) {
        case GroupRoleQS::Owner: return "owner";
        case GroupRoleQS::Admin: return "admin";
        case GroupRoleQS::Member: return "member";
    }
    return "member";
}
}  // namespace

JsonQS GroupMemberQS::toJson() const {
    return JsonQS{{"account", account_}, {"role", roleKey(role_)}, {"joinedOn", joinedOn_}};
}

GroupMemberQS GroupMemberQS::fromJson(const JsonQS& j) {
    return GroupMemberQS(j.value("account", std::string()), roleFromText(j.value("role", std::string("member"))),
                         j.value("joinedOn", std::string()));
}

}  // namespace qs
