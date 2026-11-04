#include "ServiceSelectPageQS.h"

#include "AvatarQS.h"
#include "LayoutUtilQS.h"
#include "BusLogDialogQS.h"
#include "IconQS.h"
#include "PathQS.h"
#include "ThemeQS.h"

#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPushButton>
#include <QVBoxLayout>

namespace qs {

namespace {

QString q(const std::string& s) { return QString::fromStdString(s); }

/** 一张可点击的服务卡片。 */
class ServiceCardQS : public QWidget {
public:
    ServiceCardQS(ServiceBaseQS& s, const LoginSessionQS& session, std::function<void()> onClick, QWidget* parent)
        : QWidget(parent), onClick_(std::move(onClick)) {
        const QColor accent(q(s.accentColor()));
        setObjectName("ServiceCard");
        setAttribute(Qt::WA_StyledBackground);
        setCursor(Qt::PointingHandCursor);
        setFixedSize(300, 172);
        setStyleSheet(QString("#ServiceCard{background:white;border:1px solid #E8EAED;border-radius:18px;}"
                              "#ServiceCard:hover{border:2px solid %1;}"
                              "QLabel{background:transparent;}")
                          .arg(accent.name()));
        auto* l = new QVBoxLayout(this);
        l->setContentsMargins(22, 20, 22, 18);
        l->setSpacing(8);
        auto* top = new QHBoxLayout();
        auto* logo = new QLabel(this);
        logo->setPixmap(AvatarQS::logo(accent, q(s.logoText()), 52));
        auto* name = new QLabel(q(s.name()), this);
        name->setStyleSheet("font-size:20px;font-weight:700;");
        QString pill;
        QString pillStyle;
        if (session.isLoggedIn(s.id())) {
            pill = QString::fromUtf8("已登录 · ") + q(session.accountOf(s.id()));
            pillStyle = QString("background:%1;color:white;").arg(accent.name());
        } else if (session.isPending(s.id())) {
            pill = QString::fromUtf8("已开通 · 一键进入");
            pillStyle = QString("background:%1;color:%2;").arg(ThemeQS::softer(accent).name(), ThemeQS::dark(accent).name());
        } else {
            pill = QString::fromUtf8("未登录");
            pillStyle = "background:#F1F2F4;color:#8A8F99;";
        }
        auto* status = new QLabel(pill, this);
        status->setStyleSheet(pillStyle + "border-radius:10px;padding:3px 10px;font-size:12px;font-weight:600;");
        top->addWidget(logo);
        top->addSpacing(8);
        top->addWidget(name);
        top->addStretch(1);
        top->addWidget(status, 0, Qt::AlignTop);
        l->addLayout(top);
        auto* slogan = new QLabel(q(s.slogan()), this);
        slogan->setWordWrap(true);
        slogan->setStyleSheet("color:#4A505A;font-size:13px;");
        l->addWidget(slogan);
        auto* rule = new QLabel(q(s.idRuleText()), this);
        rule->setWordWrap(true);
        rule->setStyleSheet("color:#9AA0A9;font-size:11px;");
        l->addWidget(rule);
        l->addStretch(1);
    }

protected:
    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton && rect().contains(event->pos())) onClick_();
    }

private:
    std::function<void()> onClick_;
};

}  // namespace

ServiceSelectPageQS::ServiceSelectPageQS(ClientQS& client, PickQS onPick, std::function<void()> onSwitchUser, QWidget* parent)
    : QWidget(parent), client_(client), onPick_(std::move(onPick)) {
    setObjectName("SelectPage");
    setAttribute(Qt::WA_StyledBackground);
    setStyleSheet(ThemeQS::styleSheet(QColor("#1677FF")) + "#SelectPage{background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #F4F7FB,stop:1 #EEF1F6);}");
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(40, 36, 40, 28);
    outer->addStretch(1);

    auto* brand = new QHBoxLayout();
    brand->addStretch(1);
    auto* appLogo = new QLabel(this);
    const QPixmap png(q(PathQS::toUtf8(PathQS::executableDirectory() / "TongChuangQS.png")));
    appLogo->setPixmap(png.isNull() ? AvatarQS::logo(QColor("#1677FF"), QString::fromUtf8("同"), 64)
                                    : png.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    auto* titles = new QVBoxLayout();
    auto* title = new QLabel(QString::fromUtf8("同窗 · 立体社交平台"), this);
    title->setStyleSheet("font-size:28px;font-weight:700;background:transparent;");
    auto* sub = new QLabel(QString::fromUtf8("请选择要使用的服务，再登录该服务的账号"), this);
    sub->setStyleSheet("font-size:14px;color:#6B7280;background:transparent;");
    titles->addWidget(title);
    titles->addWidget(sub);
    brand->addWidget(appLogo);
    brand->addSpacing(14);
    brand->addLayout(titles);
    brand->addStretch(1);
    outer->addLayout(brand);
    outer->addSpacing(30);

    auto* gridRow = new QHBoxLayout();
    gridRow->addStretch(1);
    grid_ = new QGridLayout();
    grid_->setHorizontalSpacing(20);
    grid_->setVerticalSpacing(20);
    gridRow->addLayout(grid_);
    gridRow->addStretch(1);
    outer->addLayout(gridRow);
    outer->addSpacing(24);

    auto* bottom = new QHBoxLayout();
    bottom->addStretch(1);
    identity_ = new QLabel(this);
    identity_->setStyleSheet("color:#6B7280;background:transparent;");
    auto* log = new QPushButton(QString::fromUtf8("平台日志"), this);
    auto* switchUser = new QPushButton(QString::fromUtf8("切换用户"), this);
    bottom->addWidget(identity_);
    bottom->addSpacing(12);
    bottom->addWidget(log);
    bottom->addWidget(switchUser);
    bottom->addStretch(1);
    outer->addLayout(bottom);
    outer->addStretch(1);
    auto* footer = new QLabel(QString::fromUtf8("面向对象程序课程设计 · 模拟即时通信系统 · TongChuangQS v3"), this);
    footer->setAlignment(Qt::AlignCenter);
    footer->setStyleSheet("color:#A0A5AD;font-size:11px;background:transparent;");
    outer->addWidget(footer);

    connect(log, &QPushButton::clicked, this, [this] {
        BusLogDialogQS dialog(client_.platform(), this);
        dialog.exec();
    });
    connect(switchUser, &QPushButton::clicked, this, [onSwitchUser] { onSwitchUser(); });
    refresh();
}

void ServiceSelectPageQS::refresh() {
    LayoutUtilQS::clear(grid_);
    const auto& services = client_.platform().services();
    const LoginSessionQS& session = client_.platform().session();
    const int columns = services.size() > 4 ? 3 : 2;
    int index = 0;
    for (const auto& s : services) {
        const std::string id = s->id();
        auto* card = new ServiceCardQS(*s, session, [this, id] { onPick_(id); }, this);
        grid_->addWidget(card, index / columns, index % columns);
        ++index;
    }
    QStringList logged;
    for (const auto& ref : session.loggedInAccounts()) {
        ServiceBaseQS* s = client_.platform().service(ref.service());
        logged << q(s->name() + " " + ref.account());
    }
    identity_->setText(logged.isEmpty() ? QString::fromUtf8("当前未登录任何服务")
                                        : QString::fromUtf8("已登录：") + logged.join(QString::fromUtf8("、")));
}

}  // namespace qs
