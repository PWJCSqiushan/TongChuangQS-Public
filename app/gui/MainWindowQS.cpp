#include "MainWindowQS.h"

#include "AuthPageQS.h"
#include "FormDialogQS.h"
#include "ServiceSelectPageQS.h"
#include "ServiceShellQS.h"

#include <QApplication>

namespace qs {

namespace {
QString q(const std::string& s) { return QString::fromStdString(s); }
}

MainWindowQS::MainWindowQS(ClientQS& client, const QString& windowTag) : client_(client), tag_(windowTag) {
    stack_ = new QStackedWidget(this);
    setCentralWidget(stack_);
    select_ = new ServiceSelectPageQS(client_, [this](const std::string& s) { pickService(s); }, [this] { switchUser(); }, stack_);
    auth_ = new AuthPageQS(client_, [this](const std::string& s, const QString& m) { enterShell(s, m); }, [this] { showSelect(); },
                           stack_);
    stack_->addWidget(select_);
    stack_->addWidget(auth_);
    connect(&timer_, &QTimer::timeout, this, [this] { pump(); });
    timer_.start(60);
    showSelect();
}

void MainWindowQS::updateTitle() {
    QString title = QString::fromUtf8("同窗");
    if (!tag_.isEmpty()) title += QString::fromUtf8(" · ") + tag_;
    if (auto* shell = dynamic_cast<ServiceShellQS*>(stack_->currentWidget())) {
        ServiceBaseQS* s = client_.platform().service(shell->service());
        const std::string me = client_.accountOf(shell->service());
        title += QString::fromUtf8(" — %1 · %2（%3）").arg(q(s->name()), q(s->nicknameOf(me)), q(me));
    }
    setWindowTitle(title);
}

void MainWindowQS::showSelect() {
    select_->refresh();
    stack_->setCurrentWidget(select_);
    updateTitle();
}

void MainWindowQS::pickService(const std::string& service) {
    const LoginSessionQS& session = client_.platform().session();
    if (session.isLoggedIn(service)) {
        enterShell(service, {});
        return;
    }
    if (session.isPending(service)) {
        ServiceBaseQS* s = client_.platform().service(service);
        if (FormDialogQS::confirm(select_, QString::fromUtf8("一键进入%1").arg(q(s->name())),
                                  QString::fromUtf8("你已在其他服务登录，系统发现你开通了%1账号 %2（%3）。确认后直接进入，无需再次输入密码。")
                                      .arg(q(s->name()), q(session.accountOf(service)), q(s->nicknameOf(session.accountOf(service)))),
                                  QString::fromUtf8("确认进入"))) {
            const ResultQS r = client_.confirmPending(service);
            if (r) {
                enterShell(service, q(r.message()));
                return;
            }
        }
    }
    auth_->setService(service);
    stack_->setCurrentWidget(auth_);
    updateTitle();
}

void MainWindowQS::enterShell(const std::string& service, const QString& message) {
    ServiceShellQS*& shell = shells_[service];
    if (shell == nullptr) {
        ServiceShellQS::CallbacksQS callbacks;
        callbacks.backToServices = [this] { showSelect(); };
        callbacks.logoutService = [this, service] { logoutService(service); };
        callbacks.switchUser = [this] { switchUser(); };
        shell = new ServiceShellQS(client_, service, callbacks, stack_);
        stack_->addWidget(shell);
    }
    stack_->setCurrentWidget(shell);
    shell->refreshAll();
    if (!message.isEmpty()) shell->toast(message, true);
    updateTitle();
}

void MainWindowQS::logoutService(const std::string& service) {
    client_.logout(service);
    const auto it = shells_.find(service);
    if (it != shells_.end()) {
        stack_->removeWidget(it->second);
        it->second->hide();
        it->second->deleteLater();
        shells_.erase(it);
    }
    showSelect();
}

void MainWindowQS::switchUser() {
    client_.logoutAll();
    for (auto& entry : shells_) {
        stack_->removeWidget(entry.second);
        entry.second->hide();
        entry.second->deleteLater();
    }
    shells_.clear();
    showSelect();
}

void MainWindowQS::pump() {
    bool changed = false;
    const std::vector<ChatEventQS> events = client_.pump(changed);
    for (auto& entry : shells_) entry.second->dispatchChatEvents(events);
    for (const auto& e : events) {
        if (e.kind == ChatEventQS::KindQS::MessageReceived && !isActiveWindow()) QApplication::alert(this);
    }
    if (changed) {
        for (auto& entry : shells_) entry.second->refreshAll();
        if (stack_->currentWidget() == select_) select_->refresh();
    }
    if (++ticks_ % 50 == 0) {  // 约每 3 秒刷新一次在线状态
        if (auto* shell = dynamic_cast<ServiceShellQS*>(stack_->currentWidget())) shell->refreshPresence();
    }
}

}  // namespace qs
