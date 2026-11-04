#include "PresenceBoardQS.h"

#include "DateQS.h"
#include "JsonFileQS.h"
#include "PathQS.h"

namespace qs {

PresenceBoardQS::PresenceBoardQS(const std::filesystem::path& directory) : directory_(directory) {}

std::filesystem::path PresenceBoardQS::fileFor(const AccountRefQS& ref) const {
    return directory_ / PathQS::fromUtf8(ref.service() + "__" + ref.account() + ".json");
}

void PresenceBoardQS::publish(const PresenceQS& p) const {
    JsonFileQS::write(fileFor(p.ref), JsonQS{{"service", p.ref.service()},
                                             {"account", p.ref.account()},
                                             {"nickname", p.nickname},
                                             {"host", p.host},
                                             {"port", p.port},
                                             {"pid", p.pid},
                                             {"heartbeat", DateQS::nowMillis()}});
}

void PresenceBoardQS::withdraw(const AccountRefQS& ref, long long pid, unsigned short port) const {
    const JsonQS j = JsonFileQS::read(fileFor(ref));
    if (j.value("pid", 0LL) == pid && j.value("port", 0) == port) {
        // 标记离线（不删除文件，避免依赖删除权限）。
        JsonQS copy = j;
        copy["heartbeat"] = 0;
        JsonFileQS::write(fileFor(ref), copy);
    }
}

bool PresenceBoardQS::lookup(const AccountRefQS& ref, PresenceQS& out) const {
    const JsonQS j = JsonFileQS::read(fileFor(ref));
    const std::int64_t beat = j.value("heartbeat", static_cast<std::int64_t>(0));
    if (beat <= 0 || DateQS::nowMillis() - beat > kTimeoutMs) return false;
    out.ref = ref;
    out.nickname = j.value("nickname", std::string());
    out.host = j.value("host", std::string("127.0.0.1"));
    out.port = j.value("port", static_cast<unsigned short>(0));
    out.pid = j.value("pid", 0LL);
    out.heartbeat = beat;
    return out.port != 0;
}

bool PresenceBoardQS::isOnline(const AccountRefQS& ref) const {
    PresenceQS ignored;
    return lookup(ref, ignored);
}

}  // namespace qs
