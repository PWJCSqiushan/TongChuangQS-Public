#include "ServiceBaseQS.h"

#include "DateQS.h"
#include "FileLockQS.h"
#include "JsonFileQS.h"
#include "PathQS.h"
#include <stdexcept>

namespace qs {

ServiceBaseQS::ServiceBaseQS(std::string id, std::string name) : id_(std::move(id)), name_(std::move(name)) {}

ServiceBaseQS::~ServiceBaseQS() = default;

void ServiceBaseQS::attach(ServiceBusQS& bus, const std::string& dataRootUtf8) {
    bus_ = &bus;
    dataDir_ = PathQS::fromUtf8(dataRootUtf8) / PathQS::fromUtf8(id_);
}

std::uint64_t ServiceBaseQS::readRevisionFile() const {
    if (!std::filesystem::exists(dataDir_ / "revision.json")) return 0;
    const auto root = JsonFileQS::read(dataDir_ / "revision.json", nullptr);
    if (!root.is_object() || !root.contains("revision") || !root["revision"].is_number_unsigned())
        throw std::runtime_error("版本文件损坏，已保留原文件");
    if (root.value("service", id_) != id_) throw std::runtime_error("版本文件服务不匹配");
    if (root.contains("format") || root.contains("files")) {
        if (root.value("format", 0) != 1 || !root.contains("files") || !root["files"].is_object())
            throw std::runtime_error("不支持或损坏的服务快照，已保留原文件");
        const auto expected = captureFiles();
        for (auto it = expected.begin(); it != expected.end(); ++it)
            if (!root["files"].contains(it.key())) throw std::runtime_error("服务快照缺少 " + it.key());
        const auto& accounts = root["files"]["accounts.json"];
        if (!accounts.is_array() && (!accounts.is_object() || !accounts.contains("accounts") || !accounts["accounts"].is_array()))
            throw std::runtime_error("服务快照账号结构损坏");
    }
    return root["revision"].get<std::uint64_t>();
}

ResultQS ServiceBaseQS::reserveMissingLinkedIds(const std::vector<std::string>& ids) {
    bool needed = false;
    for (const auto& id : ids) needed = needed || (!accounts_.contains(id) && !accounts_.isRetired(id));
    if (!needed) return ResultQS::ok();
    return transact([&] {
        for (const auto& id : ids) if (!accounts_.contains(id)) accounts_.reserveRetired(id);
        return ResultQS::ok();
    });
}

void ServiceBaseQS::loadAll() {
    const auto root = JsonFileQS::read(dataDir_ / "revision.json", nullptr);
    stateFiles_ = nullptr;
    if (root.is_object() && (root.contains("format") || root.contains("files"))) {
        if (root.value("format", 0) != 1 || root.value("service", std::string()) != id_ ||
            !root.contains("files") || !root["files"].is_object() || !root["files"].contains("accounts.json"))
            throw std::runtime_error("不支持或损坏的服务快照，已保留原文件");
        stateFiles_ = root["files"];
    }
    revision_ = readRevisionFile();
    accounts_.fromJson(readDataFile("accounts.json", JsonQS::array()));
    loadData();
}

JsonQS ServiceBaseQS::captureFiles() const {
    JsonQS files{{"accounts.json", accounts_.toJson()}};
    writeBuffer_ = &files;
    try { saveData(); } catch (...) { writeBuffer_ = nullptr; throw; }
    writeBuffer_ = nullptr;
    return files;
}

void ServiceBaseQS::restoreFiles(const JsonQS& files) {
    stateFiles_ = files;
    accounts_.fromJson(readDataFile("accounts.json"));
    loadData();
}

bool ServiceBaseQS::saveAll() {
    const JsonQS files = captureFiles();
    // One rename commits accounts, optional service data and revision together.
    // Existing split JSON is imported on read and retained; after the first
    // successful commit this snapshot is the sole business-data authority.
    if (!JsonFileQS::write(dataDir_ / "revision.json",
            JsonQS{{"format", 1}, {"revision", revision_}, {"service", id_}, {"files", files}})) return false;
    stateFiles_ = files;
    return true;
}

ResultQS ServiceBaseQS::load() {
    FileLockQS lock(dataDir_ / ".lock");
    if (!lock.locked()) {
        storageError_ = "数据正被另一个窗口占用，未加载或生成数据";
        return ResultQS::fail(storageError_);
    }
    try {
        if (!std::filesystem::exists(dataDir_ / "accounts.json") &&
            !std::filesystem::exists(dataDir_ / "revision.json")) {
            // Only a new directory may be seeded. Partial/corrupt stores fail closed.
            for (const auto& entry : std::filesystem::directory_iterator(dataDir_)) {
                if (entry.path().extension() == ".json") throw std::runtime_error("不完整的服务数据，未生成演示数据");
            }
            seedDemoData(); revision_ = 1;
            if (!saveAll()) throw std::runtime_error("首次保存失败：请检查数据目录是否可写");
        } else { loadAll(); }
        storageError_.clear();
        return ResultQS::ok();
    } catch (const std::exception& e) {
        storageError_ = e.what();
        return ResultQS::fail(storageError_);
    }
}

bool ServiceBaseQS::reloadIfChanged() {
    try {
        if (storageError_.empty() && readRevisionFile() == revision_) return false;
    } catch (const std::exception& e) { storageError_ = e.what(); return false; }
    FileLockQS lock(dataDir_ / ".lock");
    if (!lock.locked()) { storageError_ = "数据正被另一个窗口占用，保留原状态"; return false; }
    const auto before = captureFiles();
    const auto revision = revision_;
    try { loadAll(); storageError_.clear(); return true; }
    catch (const std::exception& e) {
        restoreFiles(before); revision_ = revision; storageError_ = e.what(); return false;
    }
}

ResultQS ServiceBaseQS::transact(const std::function<ResultQS()>& change) {
    FileLockQS lock(dataDir_ / ".lock");
    if (!lock.locked()) {
        return ResultQS::fail("数据正被另一个窗口占用，请稍后重试");
    }
    const auto beforeRefresh = captureFiles();
    const auto beforeRevision = revision_;
    try { if (!storageError_.empty() || readRevisionFile() != revision_) loadAll(); }
    catch (const std::exception& e) {
        restoreFiles(beforeRefresh); revision_ = beforeRevision;
        return ResultQS::fail(e.what());
    }
    const auto before = captureFiles();
    const auto revision = revision_;
    try {
        const ResultQS result = change();
        if (!result) { restoreFiles(before); return result; }
        ++revision_;
        if (saveAll()) { storageError_.clear(); return result; }
        restoreFiles(before); revision_ = revision;
        return ResultQS::fail("保存失败：请检查数据目录是否可写");
    } catch (const std::exception& e) {
        restoreFiles(before); revision_ = revision;
        return ResultQS::fail(std::string("操作未提交：") + e.what());
    }
}

JsonQS ServiceBaseQS::readDataFile(const std::string& name, const JsonQS& fallback) const {
    (void)fallback;
    if (stateFiles_.is_object()) {
        if (!stateFiles_.contains(name)) throw std::runtime_error("服务快照缺少 " + name);
        return stateFiles_[name];
    }
    const auto data = JsonFileQS::read(dataDir_ / PathQS::fromUtf8(name), nullptr);
    if (data.is_null()) throw std::runtime_error("服务数据缺失或损坏：" + name);
    return data;
}

bool ServiceBaseQS::writeDataFile(const std::string& name, const JsonQS& value) const {
    if (writeBuffer_ == nullptr) return false;
    (*writeBuffer_)[name] = value;
    return true;
}

std::string ServiceBaseQS::nicknameOf(const std::string& id) const {
    const ServiceAccountQS* a = accounts_.find(id);
    return a == nullptr ? id : a->profile().nickname();
}

std::vector<RegistrationFieldQS> ServiceBaseQS::registrationFields() const {
    std::vector<RegistrationFieldQS> fields = idFields();
    fields.push_back({"nickname", "昵称", "例如：丘山", false, false});
    fields.push_back({"password", "密码", "至少6位", true, false});
    fields.push_back({"birthday", "出生日期", "YYYY-MM-DD", false, false});
    fields.push_back({"location", "所在地", "例如：吉林长春", false, false});
    return fields;
}

ResultQS ServiceBaseQS::registerAccount(const FormQS& form, std::string& newId) {
    const auto get = [&](const char* key) {
        const auto it = form.find(key);
        return it == form.end() ? std::string() : it->second;
    };
    const ResultQS valid = UserProfileQS::validate(get("nickname"), get("birthday"), get("location"));
    if (!valid) {
        return valid;
    }
    if (get("password").size() < 6) {
        return ResultQS::fail("密码至少需要6位");
    }
    std::string id;
    const ResultQS prepared = prepareAccountId(form, id);  // 可变步骤：号码规则由子类决定
    if (!prepared) {
        return prepared;
    }
    const ResultQS result = transact([&]() -> ResultQS {
        finalizeAccountId(id);
        ServiceAccountQS account(id, get("password"),
                                 UserProfileQS(get("nickname"), get("birthday"), get("location"), DateQS::today()));
        const ResultQS added = mutableAccounts().add(std::move(account));
        if (!added) {
            return added;
        }
        onAccountCreated(*mutableAccounts().find(id), form);
        return ResultQS::ok(name_ + "注册成功，你的" + accountLabel() + "是 " + id);
    });
    if (result) {
        newId = id;
    }
    return result;
}

ResultQS ServiceBaseQS::login(const std::string& account, const std::string& password) {
    reloadIfChanged();
    if (!storageError_.empty()) return ResultQS::fail(storageError_);
    const ServiceAccountQS* a = accounts_.find(account);
    if (a == nullptr) {
        return ResultQS::fail("该" + accountLabel() + "尚未注册" + name_);
    }
    if (!a->checkPassword(password)) {
        return ResultQS::fail("密码错误");
    }
    return ResultQS::ok("欢迎回来，" + a->profile().nickname());
}

ResultQS ServiceBaseQS::deactivate(const std::string& accountId, const std::string& password) {
    return transact([&]() -> ResultQS {
        const ServiceAccountQS* a = accounts_.find(accountId);
        if (a == nullptr) {
            return ResultQS::fail("账号不存在");
        }
        if (!a->checkPassword(password)) {
            return ResultQS::fail("密码错误，未注销");
        }
        onAccountRemoved(accountId);
        accounts_.remove(accountId);
        return ResultQS::ok("已注销" + name_ + "账号 " + accountId + "，其他服务的账号不受影响");
    });
}

ResultQS ServiceBaseQS::updateProfile(const std::string& accountId, const std::string& field, const std::string& value) {
    return transact([&]() -> ResultQS {
        ServiceAccountQS* a = accounts_.find(accountId);
        if (a == nullptr) {
            return ResultQS::fail("账号不存在");
        }
        if (field == "nickname") return a->profile().setNickname(value);
        if (field == "birthday") return a->profile().setBirthday(value);
        if (field == "location") return a->profile().setLocation(value);
        if (field == "signature") return a->profile().setSignature(value);
        return ResultQS::fail("未知的资料项");
    });
}

ResultQS ServiceBaseQS::changePassword(const std::string& accountId, const std::string& oldPassword,
                                       const std::string& newPassword) {
    return transact([&]() -> ResultQS {
        ServiceAccountQS* a = accounts_.find(accountId);
        if (a == nullptr) return ResultQS::fail("账号不存在");
        if (!a->checkPassword(oldPassword)) return ResultQS::fail("原密码错误");
        if (newPassword.size() < 6) return ResultQS::fail("新密码至少需要6位");
        a->setPassword(newPassword);
        return ResultQS::ok("密码已修改");
    });
}

bool ServiceBaseQS::acceptsLinkedRegistrationFrom(const std::string&) const { return false; }

void ServiceBaseQS::onAccountCreated(ServiceAccountQS&, const FormQS&) {}

ResultQS ServiceBaseQS::createLinkedAccount(const UserProfileQS&, const std::string&, const AccountRefQS&, std::string&) {
    return ResultQS::fail(name_ + "不支持联动开通");
}

void ServiceBaseQS::onAccountRemoved(const std::string&) {}

bool ServiceBaseQS::onRequest(const ServiceRequestQS&, ServiceReplyQS&) { return false; }

ResultQS ServiceBaseQS::bindAccount(const std::string& mine, const AccountRefQS& other, const std::string& otherPassword) {
    if (bus_ == nullptr) {
        return ResultQS::fail("服务尚未接入平台");
    }
    bool allowed = false;
    for (const auto& s : bindableServices()) {
        allowed = allowed || s == other.service();
    }
    if (!allowed) {
        return ResultQS::fail(name_ + "不支持绑定该服务的账号");
    }
    // 核验交给对方服务完成：本服务看不到对方的账号数据。
    const ServiceReplyQS reply = bus_->send(ServiceRequestQS{
        id_, other.service(), "verify_credentials", JsonQS{{"account", other.account()}, {"password", otherPassword}}});
    if (!reply.ok) {
        return ResultQS::fail("对方服务核验未通过：" + reply.message);
    }
    return transact([&]() -> ResultQS {
        ServiceAccountQS* a = accounts_.find(mine);
        if (a == nullptr) return ResultQS::fail("账号不存在");
        if (a->isLinkedTo(other)) return ResultQS::fail("已经绑定过该账号");
        for (const auto& link : a->links()) {
            if (link.service() == other.service()) {
                return ResultQS::fail("已绑定了另一个同类账号，请先解绑");
            }
        }
        if (const ServiceAccountQS* holder = accounts_.findLinkedTo(other)) {
            return ResultQS::fail("该账号已被 " + holder->id() + " 绑定");
        }
        a->addLink(other);
        return ResultQS::ok("绑定成功");
    });
}

ResultQS ServiceBaseQS::unbindAccount(const std::string& mine, const AccountRefQS& other) {
    return transact([&]() -> ResultQS {
        ServiceAccountQS* a = accounts_.find(mine);
        if (a == nullptr || !a->isLinkedTo(other)) return ResultQS::fail("没有这个绑定");
        a->removeLink(other);
        return ResultQS::ok("已解除绑定");
    });
}

ServiceReplyQS ServiceBaseQS::handleRequest(const ServiceRequestQS& request) {
    reloadIfChanged();
    if (!storageError_.empty()) return ServiceReplyQS::failure(storageError_);
    ServiceReplyQS reply;
    if (onRequest(request, reply)) {  // 子类特有请求优先
        return reply;
    }
    const JsonQS& p = request.payload;
    if (request.kind == "verify_credentials") {
        const ServiceAccountQS* a = accounts_.find(p.value("account", std::string()));
        if (a == nullptr) return ServiceReplyQS::failure("账号不存在");
        if (!a->checkPassword(p.value("password", std::string()))) return ServiceReplyQS::failure("密码错误");
        return ServiceReplyQS::success(JsonQS{{"profile", a->profile().toJson()}});
    }
    if (request.kind == "account_info") {
        const ServiceAccountQS* a = accounts_.find(p.value("account", std::string()));
        if (a == nullptr) return ServiceReplyQS::failure("账号不存在");
        return ServiceReplyQS::success(JsonQS{{"profile", a->profile().toJson()}});
    }
    if (request.kind == "linked_accounts") {
        // 返回本服务所知道的、与 ref 有关联的账号（双向都查）。
        const AccountRefQS ref = AccountRefQS::fromJson(p.value("ref", JsonQS::object()));
        JsonQS result = JsonQS::array();
        if (ref.service() == id_) {
            if (const ServiceAccountQS* a = accounts_.find(ref.account())) {
                for (const auto& link : a->links()) result.push_back(link.toJson());
            }
        } else {
            for (const ServiceAccountQS* a : accounts_.all()) {
                if (a->isLinkedTo(ref)) result.push_back(AccountRefQS(id_, a->id()).toJson());
            }
        }
        return ServiceReplyQS::success(JsonQS{{"refs", result}});
    }
    if (request.kind == "friends_of") {
        const FriendCapabilityQS* friends = friendCapability();
        if (friends == nullptr) return ServiceReplyQS::failure(name_ + "没有好友功能");
        JsonQS list = JsonQS::array();
        for (const auto& f : friends->friendsOf(p.value("account", std::string()))) {
            list.push_back(JsonQS{{"account", f.account}, {"nickname", f.nickname}});
        }
        return ServiceReplyQS::success(JsonQS{{"friends", list}});
    }
    if (request.kind == "register_linked") {
        if (!acceptsLinkedRegistrationFrom(request.from)) {
            return ServiceReplyQS::failure(name_ + "不接受该服务的联动开通");
        }
        const UserProfileQS profile = UserProfileQS::fromJson(p.value("profile", JsonQS::object()));
        const AccountRefQS source = AccountRefQS::fromJson(p.value("source", JsonQS::object()));
        std::string newId;
        const ResultQS r = transact(
            [&]() { return createLinkedAccount(profile, p.value("password", std::string()), source, newId); });
        if (!r) return ServiceReplyQS::failure(r.message());
        return ServiceReplyQS::success(JsonQS{{"account", newId}}, r.message());
    }
    return ServiceReplyQS::failure(name_ + "无法处理请求：" + request.kind);
}

void ServiceBaseQS::seedAccount(const std::string& id, const std::string& nickname, const std::string& birthday,
                                const std::string& location, const std::string& registeredOn,
                                const std::vector<AccountRefQS>& links, const std::string& signature) {
    ServiceAccountQS account(id, "123456", UserProfileQS(nickname, birthday, location, registeredOn));
    account.profile().setSignature(signature);
    for (const auto& link : links) {
        account.addLink(link);
    }
    accounts_.add(std::move(account));
}

}  // namespace qs
