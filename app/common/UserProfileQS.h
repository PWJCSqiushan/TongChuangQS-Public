#pragma once

#include "JsonQS.h"
#include "ResultQS.h"

#include <string>

namespace qs {

/**
 * 用户基本信息：昵称、出生日期、所在地、号码申请时间（用于计算 T 龄）、个性签名。
 * 字段私有，只能通过带校验的接口修改。
 */
class UserProfileQS {
public:
    UserProfileQS() = default;
    UserProfileQS(std::string nickname, std::string birthday, std::string location, std::string registeredOn);

    const std::string& nickname() const noexcept { return nickname_; }
    const std::string& birthday() const noexcept { return birthday_; }
    const std::string& location() const noexcept { return location_; }
    const std::string& registeredOn() const noexcept { return registeredOn_; }
    const std::string& signature() const noexcept { return signature_; }

    /** T 龄：从号码申请日到今天的完整年数。 */
    int tAge() const;
    int age() const;

    ResultQS setNickname(const std::string& value);
    ResultQS setBirthday(const std::string& value);
    ResultQS setLocation(const std::string& value);
    ResultQS setSignature(const std::string& value);
    void setRegisteredOn(const std::string& value) { registeredOn_ = value; }

    /** 注册前统一校验。 */
    static ResultQS validate(const std::string& nickname, const std::string& birthday, const std::string& location);

    JsonQS toJson() const;
    static UserProfileQS fromJson(const JsonQS& j);

private:
    std::string nickname_;
    std::string birthday_;
    std::string location_;
    std::string registeredOn_;
    std::string signature_;
};

}  // namespace qs
