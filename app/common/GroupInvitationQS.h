#pragma once

#include "JsonQS.h"
#include <string>
#include <vector>

namespace qs {

class GroupInvitationQS {
public:
    std::string invitee;
    std::string inviter;
    std::string date;
    JsonQS toJson() const { return JsonQS{{"invitee", invitee}, {"inviter", inviter}, {"date", date}}; }
    static GroupInvitationQS fromJson(const JsonQS& j) {
        return GroupInvitationQS{j.value("invitee", std::string()), j.value("inviter", std::string()),
                                 j.value("date", std::string())};
    }
};

}  // namespace qs
