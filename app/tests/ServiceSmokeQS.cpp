#include "PlatformQS.h"
#include "PathQS.h"
#include "TestHarnessQS.h"
#include <chrono>

using namespace qs;
QS_TEST(CompiledServicesRemainIndependentAndUsable) {
    const auto root = std::filesystem::temp_directory_path() /
        ("qs-service-smoke-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    PlatformQS p(PathQS::toUtf8(root));
    QS_CHECK(p.start());
    QS_CHECK(!p.services().empty());
#ifdef QS_EXPECT_NO_WESHOP
    QS_CHECK(p.service("weshop") == nullptr);
#endif
    for (const auto& s : p.services()) {
        QS_CHECK(s->revision() == 1);
        QS_CHECK(s->accounts().size() > 0);
        if (auto* groups = s->groupCapability()) {
            GroupQS group;
            for (int id = 1001; id <= 1006; ++id) QS_CHECK(groups->findGroup(std::to_string(id), group));
        }
        if (!s->accounts().all().empty()) {
            const auto id = s->accounts().all().front()->id();
            QS_CHECK(p.login(s->id(), id, "123456"));
        }
    }
    PlatformQS reopened(PathQS::toUtf8(root));
    QS_CHECK(reopened.start());
    QS_CHECK(reopened.serviceIds() == p.serviceIds());
}
int main() { return TestHarnessQS::instance().run(); }
