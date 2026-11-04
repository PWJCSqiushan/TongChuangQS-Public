#pragma once

#include "AccountRefQS.h"

#include <map>
#include <string>
#include <vector>

namespace qs {

/** 某服务在本窗口中的登录状态。 */
enum class SessionStateQS {
    LoggedIn,  // 已登录
    Pending    // 已开通、待确认（其他服务登录后自动发现，简单确认即可进入）
};

/**
 * 本窗口（本进程）的登录会话：一个"人"在各服务中的登录情况。
 * 题目要求"各微X之间只要有一个服务登录，则其它服务简单确认后视为自动登录"。
 */
class LoginSessionQS {
public:
    bool isLoggedIn(const std::string& service) const;
    bool isPending(const std::string& service) const;
    /** 返回该服务对应的账号（已登录或待确认）；没有则返回空串。 */
    std::string accountOf(const std::string& service) const;
    std::vector<AccountRefQS> loggedInAccounts() const;
    bool anyLoggedIn() const;

    void markLoggedIn(const std::string& service, const std::string& account);
    void markPending(const std::string& service, const std::string& account);
    void logout(const std::string& service);
    void clear();

private:
#include "EntryQS.h"
    std::map<std::string, EntryQS> entries_;
};

}  // namespace qs
