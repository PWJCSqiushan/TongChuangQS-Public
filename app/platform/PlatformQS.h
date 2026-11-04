#pragma once

#include "LoginSessionQS.h"
#include "ServiceBaseQS.h"

#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "BusLogEntryQS.h"
#include "FriendCandidateQS.h"
#include "CommonFriendQS.h"

namespace qs {

/** 服务总线上的一条通信记录（界面"平台日志"中展示，答辩时可直观看到服务间只通过消息协作）。 */


/** 跨服务推荐的一个好友候选。 */


/** 两个服务里都是好友的人（共同好友）。 */


/**
 * 立体社交平台。职责：
 *  - 启动时向 ServiceRegistryQS 取得全部服务并加载（不认识任何具体服务类）；
 *  - 作为服务总线转发服务间请求，并记录通信日志；
 *  - 维护本窗口的登录会话，实现"一处登录、其他服务确认即进入"；
 *  - 提供依赖多个服务协作的平台级功能（联动注册、跨服务推荐好友、共同好友）。
 */
class PlatformQS final : public ServiceBusQS {
public:
    explicit PlatformQS(std::string dataRootUtf8);
    ~PlatformQS() override;

    ResultQS start();
    const std::string& dataRoot() const noexcept { return dataRoot_; }

    const std::vector<std::unique_ptr<ServiceBaseQS>>& services() const noexcept { return services_; }
    ServiceBaseQS* service(const std::string& id) const;

    ServiceReplyQS send(const ServiceRequestQS& request) override;
    std::vector<std::string> serviceIds() const override;
    const std::deque<BusLogEntryQS>& busLog() const noexcept { return busLog_; }

    /** 其他窗口修改了数据时重新读取；返回是否有服务发生变化。 */
    bool refreshAll();

    // ———— 登录会话 ————
    const LoginSessionQS& session() const noexcept { return session_; }
    ResultQS login(const std::string& serviceId, const std::string& account, const std::string& password);
    /** "已开通待确认"的服务经用户确认后直接进入。 */
    ResultQS confirmPending(const std::string& serviceId);
    void logout(const std::string& serviceId);
    void logoutAll();

    // ———— 跨服务功能 ————
    /** 与 ref 关联的全部账号（含间接关联，如 微商→微信→QQ）。 */
    std::vector<AccountRefQS> linkedAccounts(const AccountRefQS& ref);
    /** 我在 targetService 中关联的账号，没有返回空串。 */
    std::string linkedAccountIn(const AccountRefQS& me, const std::string& targetService);
    /** 注册 source 服务时可以顺带开通的其他服务。 */
    std::vector<std::string> linkedRegistrationTargets(const std::string& sourceService) const;
    /** 联动开通：平台把请求转给目标服务，由目标服务自己完成注册。 */
    std::vector<std::pair<std::string, ResultQS>> registerLinked(const AccountRefQS& source, const std::string& password,
                                                                 const std::vector<std::string>& targets);
    /** 依据我在 sourceService 中的好友，推荐 targetService 中可添加的好友。 */
    std::vector<FriendCandidateQS> recommendFriends(const std::string& targetService, const std::string& me,
                                                    const std::string& sourceService, std::string& hint);
    /** 我在两个服务中的共同好友。 */
    std::vector<CommonFriendQS> commonFriends(const AccountRefQS& me, const std::string& otherService, std::string& hint);

private:
    void discoverLinkedServices(const AccountRefQS& loggedIn);
    std::vector<AccountRefQS> directLinks(const AccountRefQS& ref);

    std::string dataRoot_;
    std::vector<std::unique_ptr<ServiceBaseQS>> services_;
    LoginSessionQS session_;
    std::deque<BusLogEntryQS> busLog_;
};

}  // namespace qs
