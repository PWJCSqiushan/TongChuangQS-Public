#pragma once

#include "FeaturePageQS.h"

#include <QLabel>
#include <QStackedWidget>
#include <QToolButton>

#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace qs {

class NavButtonQS;
class ToastQS;

/**
 * 某个服务登录后的主界面（三栏外壳）：左侧导航栏 + 功能页面区。
 * 导航项完全来自 ServiceBaseQS::features()（多态：每个服务给出不同的功能列表），
 * 每项由 GuiFeatureRegistryQS 创建对应页面；外壳本身不认识任何具体服务。
 */
class ServiceShellQS : public QWidget {
public:
#include "CallbacksQS.h"

    ServiceShellQS(ClientQS& client, const std::string& service, CallbacksQS callbacks, QWidget* parent);

    const std::string& service() const { return context_.service; }
    void refreshAll();
    void refreshPresence();
    void dispatchChatEvents(const std::vector<ChatEventQS>& events);
    void toast(const QString& message, bool ok);
    void showFeature(const std::string& kind, const std::string& arg = "");
    /** 截图模式：依次切换到每个功能页。 */
    std::vector<std::string> featureKinds() const;

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void updateBadges();
    void updateAvatar();

    ShellContextQS context_;
    CallbacksQS callbacks_;
    QToolButton* avatar_;
    QStackedWidget* stack_;
    std::vector<std::pair<std::string, FeaturePageQS*>> pages_;
    std::map<std::string, NavButtonQS*> buttons_;
    std::map<std::string, bool> dirty_;
    ToastQS* toast_;
};

}  // namespace qs
