#include "UserProfileQS.h"

#include "DateQS.h"

namespace qs {

UserProfileQS::UserProfileQS(std::string nickname, std::string birthday, std::string location, std::string registeredOn)
    : nickname_(std::move(nickname)),
      birthday_(std::move(birthday)),
      location_(std::move(location)),
      registeredOn_(std::move(registeredOn)) {}

int UserProfileQS::tAge() const { return DateQS::fullYearsBetween(registeredOn_, DateQS::today()); }
int UserProfileQS::age() const { return DateQS::fullYearsBetween(birthday_, DateQS::today()); }

ResultQS UserProfileQS::validate(const std::string& nickname, const std::string& birthday, const std::string& location) {
    if (nickname.empty()) {
        return ResultQS::fail("昵称不能为空");
    }
    if (nickname.size() > 48) {
        return ResultQS::fail("昵称过长（最多16个汉字）");
    }
    if (!DateQS::isValid(birthday)) {
        return ResultQS::fail("出生日期格式应为 YYYY-MM-DD，例如 2006-05-20");
    }
    if (birthday > DateQS::today()) {
        return ResultQS::fail("出生日期不能晚于今天");
    }
    if (location.empty()) {
        return ResultQS::fail("所在地不能为空");
    }
    return ResultQS::ok();
}

ResultQS UserProfileQS::setNickname(const std::string& value) {
    if (value.empty() || value.size() > 48) {
        return ResultQS::fail("昵称应为1到16个汉字");
    }
    nickname_ = value;
    return ResultQS::ok("昵称已更新");
}

ResultQS UserProfileQS::setBirthday(const std::string& value) {
    if (!DateQS::isValid(value) || value > DateQS::today()) {
        return ResultQS::fail("出生日期格式应为 YYYY-MM-DD 且不晚于今天");
    }
    birthday_ = value;
    return ResultQS::ok("出生日期已更新");
}

ResultQS UserProfileQS::setLocation(const std::string& value) {
    if (value.empty()) {
        return ResultQS::fail("所在地不能为空");
    }
    location_ = value;
    return ResultQS::ok("所在地已更新");
}

ResultQS UserProfileQS::setSignature(const std::string& value) {
    if (value.size() > 180) {
        return ResultQS::fail("个性签名最多60个汉字");
    }
    signature_ = value;
    return ResultQS::ok("个性签名已更新");
}

JsonQS UserProfileQS::toJson() const {
    return JsonQS{{"nickname", nickname_},
                  {"birthday", birthday_},
                  {"location", location_},
                  {"registeredOn", registeredOn_},
                  {"signature", signature_}};
}

UserProfileQS UserProfileQS::fromJson(const JsonQS& j) {
    UserProfileQS p(j.value("nickname", std::string()), j.value("birthday", std::string()),
                    j.value("location", std::string()), j.value("registeredOn", std::string()));
    p.signature_ = j.value("signature", std::string());
    return p;
}

}  // namespace qs
