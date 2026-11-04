#pragma once

#include "AccountBookQS.h"
#include "FriendCapabilityQS.h"
#include "GroupCapabilityQS.h"
#include "ServiceBusQS.h"
#include "ServiceDescriptorsQS.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace qs {

/**
 * 所有"微X服务"的抽象基类——平台唯一认识的服务类型。
 *
 * 设计要点：
 *  1. 以服务为核心：每个服务自己管理自己的账号、好友、群和特色功能，
 *     数据保存在 data/<服务id>/ 下，互相独立。
 *  2. 平台只通过本类的虚函数与服务交互，PlatformQS 不包含任何具体服务的头文件；
 *     新服务继承本类、在自己的 .cpp 中用 QS_REGISTER_SERVICE 自注册即可接入。
 *  3. 服务之间互不透明，只能经 ServiceBusQS 发请求（handleRequest 处理别人发来的请求）。
 *  4. 好友、群等是"可选能力"（friendCapability/groupCapability 返回 nullptr 表示不支持），
 *     而不是强加给所有服务的父类。
 *
 * 注册流程用"模板方法"：registerAccount 固定了"校验公共字段 → 服务分配号码 →
 * 写入账号 → 服务后续处理"的骨架，号码规则等可变步骤由子类覆盖虚函数实现。
 */
class ServiceBaseQS {
public:
    ServiceBaseQS(std::string id, std::string name);
    virtual ~ServiceBaseQS();

    ServiceBaseQS(const ServiceBaseQS&) = delete;
    ServiceBaseQS& operator=(const ServiceBaseQS&) = delete;

    const std::string& id() const noexcept { return id_; }
    const std::string& name() const noexcept { return name_; }

    // ———— 服务的"自我介绍"（纯虚：每个服务必须给出） ————
    virtual std::string accentColor() const = 0;   // 主题色 "#RRGGBB"
    virtual std::string logoText() const = 0;      // 标志字
    virtual std::string slogan() const = 0;        // 选择页上的一句话
    virtual std::string accountLabel() const = 0;  // "QQ号" / "微信号"
    virtual std::string idRuleText() const = 0;    // 号码规则说明
    virtual int displayOrder() const = 0;
    virtual std::vector<ServiceFeatureQS> features() const = 0;

    // ———— 生命周期与持久化 ————
    void attach(ServiceBusQS& bus, const std::string& dataRootUtf8);
    /** 读入数据；若是第一次运行则生成演示数据并保存。 */
    ResultQS load();
    ResultQS reserveMissingLinkedIds(const std::vector<std::string>& ids);
    /** 另一个进程修改过本服务数据时重新读入；返回是否发生了重读。 */
    bool reloadIfChanged();
    std::uint64_t revision() const noexcept { return revision_; }
    const std::string& storageError() const noexcept { return storageError_; }
    /**
     * 以"事务"方式修改数据：加跨进程锁 → 读取最新数据 → 执行修改 →
     * 成功则写回并递增版本号，失败则从磁盘重读以撤销半途修改。
     */
    ResultQS transact(const std::function<ResultQS()>& change);
    std::filesystem::path dataDir() const { return dataDir_; }

    // ———— 账号（模板方法） ————
    std::vector<RegistrationFieldQS> registrationFields() const;
    ResultQS registerAccount(const FormQS& form, std::string& newId);
    virtual ResultQS login(const std::string& account, const std::string& password);
    ResultQS deactivate(const std::string& account, const std::string& password);
    ResultQS updateProfile(const std::string& account, const std::string& field, const std::string& value);
    ResultQS changePassword(const std::string& account, const std::string& oldPassword, const std::string& newPassword);

    const AccountBookQS& accounts() const noexcept { return accounts_; }
    const ServiceAccountQS* account(const std::string& id) const { return accounts_.find(id); }
    std::string nicknameOf(const std::string& id) const;

    // ———— 可选能力（默认不支持） ————
    virtual FriendCapabilityQS* friendCapability() { return nullptr; }
    virtual const FriendCapabilityQS* friendCapability() const { return nullptr; }
    virtual GroupCapabilityQS* groupCapability() { return nullptr; }
    virtual const GroupCapabilityQS* groupCapability() const { return nullptr; }
    virtual bool supportsChat() const { return false; }

    // ———— 跨服务协作 ————
    /** 是否接受"其他服务注册时顺带开通本服务"。 */
    virtual bool acceptsLinkedRegistrationFrom(const std::string& fromService) const;
    /** 顺带开通时给用户的说明。 */
    virtual std::string linkedRegistrationHint() const { return ""; }
    /** 本服务允许绑定哪些服务的账号（如微信可绑定 QQ）。 */
    virtual std::vector<std::string> bindableServices() const { return {}; }
    /** 绑定：先经总线让对方服务核验账号密码，再在本服务记录绑定关系。 */
    ResultQS bindAccount(const std::string& mine, const AccountRefQS& other, const std::string& otherPassword);
    ResultQS unbindAccount(const std::string& mine, const AccountRefQS& other);
    /** 处理其他服务经总线发来的请求（非虚：先交给子类的 onRequest，再处理通用请求）。 */
    ServiceReplyQS handleRequest(const ServiceRequestQS& request);

protected:
    ServiceBusQS* bus() const noexcept { return bus_; }
    AccountBookQS& mutableAccounts() noexcept { return accounts_; }

    // ———— 子类可覆盖的步骤 ————
    /** 注册表单中与号码相关的字段（QQ 自动分配，因此为空；微信需自定义微信号）。 */
    virtual std::vector<RegistrationFieldQS> idFields() const { return {}; }
    /** 在事务外准备号码（可以经总线询问其他服务，例如微博核验 QQ 号）。 */
    virtual ResultQS prepareAccountId(const FormQS& form, std::string& id) = 0;
    /** Reallocate automatic IDs under the write lock, after external validation. */
    virtual void finalizeAccountId(std::string&) const {}
    /** 账号写入后的服务特有处理（例如记录绑定）。在事务内调用。 */
    virtual void onAccountCreated(ServiceAccountQS& account, const FormQS& form);
    /** 由其他服务发起的联动注册。在事务内调用。 */
    virtual ResultQS createLinkedAccount(const UserProfileQS& profile, const std::string& password,
                                         const AccountRefQS& source, std::string& newId);
    /** 注销账号时清理本服务内与其相关的数据。在事务内调用。 */
    virtual void onAccountRemoved(const std::string& id);
    /** 子类特有请求；返回 true 表示已处理。 */
    virtual bool onRequest(const ServiceRequestQS& request, ServiceReplyQS& reply);

    /** 子类读写自己的数据文件（账号文件由基类负责）。 */
    virtual void loadData() = 0;
    virtual void saveData() const = 0;
    /** 第一次运行时生成演示数据（每个服务自己决定演示内容）。 */
    virtual void seedDemoData() = 0;

    JsonQS readDataFile(const std::string& name, const JsonQS& fallback = JsonQS::object()) const;
    bool writeDataFile(const std::string& name, const JsonQS& value) const;
    /** 种子数据辅助：直接创建账号。 */
    void seedAccount(const std::string& id, const std::string& nickname, const std::string& birthday,
                     const std::string& location, const std::string& registeredOn,
                     const std::vector<AccountRefQS>& links = {}, const std::string& signature = "");

private:
    void loadAll();
    bool saveAll();
    JsonQS captureFiles() const;
    void restoreFiles(const JsonQS& files);
    std::uint64_t readRevisionFile() const;

    std::string id_;
    std::string name_;
    ServiceBusQS* bus_ = nullptr;
    std::filesystem::path dataDir_;
    AccountBookQS accounts_;
    std::uint64_t revision_ = 0;
    JsonQS stateFiles_; // committed per-service snapshot; null means legacy split files
    mutable JsonQS* writeBuffer_ = nullptr;
    std::string storageError_;
};

}  // namespace qs
