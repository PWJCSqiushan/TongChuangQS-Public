#include "ServiceShellQS.h"

#include "AvatarQS.h"
#include "IconQS.h"
#include "NavButtonQS.h"
#include "ThemeQS.h"
#include "ToastQS.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QMenu>
#include <QTimer>
#include <QVBoxLayout>

namespace qs {

ServiceShellQS::ServiceShellQS(ClientQS& client, const std::string& service, CallbacksQS callbacks, QWidget* parent)
    : QWidget(parent), callbacks_(std::move(callbacks)) {
    ServiceBaseQS& svc = *client.platform().service(service);
    context_.client = &client;
    context_.service = service;
    context_.accent = QColor(QString::fromStdString(svc.accentColor()));
    context_.toast = [this](const QString& m, bool ok) { toast(m, ok); };
    context_.refreshAll = [this] { QTimer::singleShot(0, this, [this] { refreshAll(); }); };
    context_.openConversation = [this](const std::string& key) { showFeature("chats", key); };
    context_.navigate = [this](const std::string& kind, const std::string& arg) { showFeature(kind, arg); };
    context_.logoutService = [this] { QTimer::singleShot(0, this, [this] { callbacks_.logoutService(); }); };
    context_.switchUser = [this] { QTimer::singleShot(0, this, [this] { callbacks_.switchUser(); }); };
    setStyleSheet(ThemeQS::styleSheet(context_.accent));

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // —— 左侧导航栏 ——
    auto* rail = new QWidget(this);
    rail->setObjectName("NavRail");
    rail->setAttribute(Qt::WA_StyledBackground);
    rail->setFixedWidth(76);
    auto* r = new QVBoxLayout(rail);
    r->setContentsMargins(8, 16, 8, 14);
    r->setSpacing(6);
    avatar_ = new QToolButton(rail);
    avatar_->setObjectName("NavButton");
    avatar_->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    avatar_->setIconSize(QSize(42, 42));
    avatar_->setFixedSize(60, 70);
    avatar_->setCursor(Qt::PointingHandCursor);
    r->addWidget(avatar_, 0, Qt::AlignHCenter);
    auto* logo = new QLabel(rail);
    logo->setPixmap(AvatarQS::logo(context_.accent, QString::fromStdString(svc.logoText()), 22, 6));
    logo->setToolTip(QString::fromStdString(svc.name()));
    logo->setAlignment(Qt::AlignCenter);
    r->addWidget(logo, 0, Qt::AlignHCenter);
    r->addSpacing(10);

    stack_ = new QStackedWidget(this);
    for (const ServiceFeatureQS& f : svc.features()) {
        FeaturePageQS* page = GuiFeatureRegistryQS::instance().create(f.kind, context_, stack_);
        if (page == nullptr) continue;  // 没有界面实现的功能不显示
        stack_->addWidget(page);
        pages_.emplace_back(f.kind, page);
        auto* button = new NavButtonQS(QString::fromStdString(f.icon), QString::fromStdString(f.title), context_.accent, rail);
        r->addWidget(button, 0, Qt::AlignHCenter);
        buttons_[f.kind] = button;
        const std::string kind = f.kind;
        connect(button, &QToolButton::clicked, this, [this, kind] { showFeature(kind); });
    }
    r->addStretch(1);
    auto* services = new NavButtonQS("grid", QString::fromUtf8("服务"), context_.accent, rail);
    services->setCheckable(false);
    services->setToolTip(QString::fromUtf8("返回服务选择（保持登录）"));
    r->addWidget(services, 0, Qt::AlignHCenter);
    auto* exit = new NavButtonQS("logout", QString::fromUtf8("退出"), context_.accent, rail);
    exit->setCheckable(false);
    r->addWidget(exit, 0, Qt::AlignHCenter);
    layout->addWidget(rail);
    layout->addWidget(stack_, 1);

    connect(services, &QToolButton::clicked, this, [this] { callbacks_.backToServices(); });
    connect(exit, &QToolButton::clicked, this, [this, exit, svcName = QString::fromStdString(svc.name())] {
        QMenu menu(this);
        QAction* one = menu.addAction(QString::fromUtf8("退出%1（其他服务保持登录）").arg(svcName));
        QAction* all = menu.addAction(QString::fromUtf8("切换用户（退出全部服务）"));
        QAction* chosen = menu.exec(exit->mapToGlobal(QPoint(exit->width() + 4, 0)));
        if (chosen == one) context_.logoutService();
        if (chosen == all) context_.switchUser();
    });
    connect(avatar_, &QToolButton::clicked, this, [this] { showFeature("me"); });

    toast_ = new ToastQS(this);
    updateAvatar();
    if (!pages_.empty()) showFeature(pages_.front().first);
    updateBadges();
}

std::vector<std::string> ServiceShellQS::featureKinds() const {
    std::vector<std::string> kinds;
    for (const auto& p : pages_) kinds.push_back(p.first);
    return kinds;
}

void ServiceShellQS::updateAvatar() {
    const std::string me = context_.me();
    const QString nick = QString::fromStdString(context_.svc().nicknameOf(me));
    avatar_->setIcon(QIcon(AvatarQS::person(QString::fromStdString(me), nick, 42)));
    avatar_->setText(nick);
    avatar_->setToolTip(QString::fromStdString(context_.svc().accountLabel() + " " + me));
}

void ServiceShellQS::showFeature(const std::string& kind, const std::string& arg) {
    for (auto& p : pages_) {
        if (p.first != kind) continue;
        stack_->setCurrentWidget(p.second);
        if (dirty_[kind]) {
            dirty_[kind] = false;
            p.second->refresh();
        }
        if (!arg.empty()) p.second->activate(arg);
    }
    for (auto& b : buttons_) b.second->setChecked(b.first == kind);
    updateBadges();
}

void ServiceShellQS::updateBadges() {
    for (auto& p : pages_) {
        const auto it = buttons_.find(p.first);
        if (it != buttons_.end()) it->second->setBadge(p.second->badgeCount());
    }
}

void ServiceShellQS::refreshAll() {
    if (QApplication::activePopupWidget() != nullptr || QApplication::activeModalWidget() != nullptr) {
        // 有菜单或对话框打开时不重建页面，稍后再刷新
        QTimer::singleShot(400, this, [this] { refreshAll(); });
        return;
    }
    if (context_.svc().account(context_.me()) == nullptr) return;
    updateAvatar();
    for (auto& p : pages_) {
        if (stack_->currentWidget() == p.second) p.second->refresh();
        else dirty_[p.first] = true;  // 切换到该页时再刷新
    }
    updateBadges();
}

void ServiceShellQS::refreshPresence() {
    if (QApplication::activePopupWidget() != nullptr || QApplication::activeModalWidget() != nullptr) return;
    if (auto* page = dynamic_cast<FeaturePageQS*>(stack_->currentWidget())) page->refreshPresence();
}

void ServiceShellQS::dispatchChatEvents(const std::vector<ChatEventQS>& events) {
    if (events.empty()) return;
    for (auto& p : pages_) p.second->onChatEvents(events);
    updateBadges();
}

void ServiceShellQS::toast(const QString& message, bool ok) { toast_->showMessage(message, ok); }

void ServiceShellQS::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (toast_->isVisible()) toast_->move((width() - toast_->width()) / 2, height() - toast_->height() - 96);
}

}  // namespace qs
