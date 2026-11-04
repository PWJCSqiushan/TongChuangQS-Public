#include "ProfilePageQS.h"
#include "LayoutUtilQS.h"

#include "AvatarQS.h"
#include "BusLogDialogQS.h"
#include "FormDialogQS.h"
#include "IconQS.h"
#include "ThemeQS.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>

namespace qs {

namespace {
QString q(const std::string& s) { return QString::fromStdString(s); }
QLabel* label(const QString& text, const char* objectName, QWidget* parent) {
    auto* l = new QLabel(text, parent);
    l->setObjectName(objectName);
    l->setWordWrap(true);
    return l;
}
QWidget* makeCard(QWidget* parent, QVBoxLayout*& inner, const QString& title) {
    auto* c = new QWidget(parent);
    c->setObjectName("Card");
    c->setAttribute(Qt::WA_StyledBackground);
    inner = new QVBoxLayout(c);
    inner->setContentsMargins(24, 18, 24, 18);
    inner->setSpacing(10);
    if (!title.isEmpty()) inner->addWidget(label(title, "RowTitle", c));
    return c;
}
}  // namespace

ProfilePageQS::ProfilePageQS(ShellContextQS& context, QWidget* parent) : FeaturePageQS(context, parent) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setObjectName("ContentPanel");
    body_ = new QWidget(scroll);
    body_->setObjectName("ContentPanel");
    body_->setAttribute(Qt::WA_StyledBackground);
    layout_ = new QVBoxLayout(body_);
    layout_->setContentsMargins(0, 28, 0, 28);
    scroll->setWidget(body_);
    outer->addWidget(scroll);
    rebuild();
}

void ProfilePageQS::refresh() { rebuild(); }

void ProfilePageQS::rebuild() {
    LayoutUtilQS::clear(layout_);
    ServiceBaseQS& s = context_.svc();
    const std::string me = context_.me();
    const ServiceAccountQS* a = s.account(me);
    if (a == nullptr) return;
    const UserProfileQS& p = a->profile();

    auto* column = new QVBoxLayout();
    column->setSpacing(16);
    auto* center = new QHBoxLayout();
    center->addStretch(1);
    center->addLayout(column, 0);
    center->addStretch(1);
    layout_->addLayout(center);
    layout_->addStretch(1);
    const int width = 640;

    // —— 资料卡 ——
    QVBoxLayout* inner = nullptr;
    QWidget* head = makeCard(body_, inner, "");
    head->setFixedWidth(width);
    auto* top = new QHBoxLayout();
    top->setSpacing(18);
    auto* avatar = new QLabel(head);
    avatar->setPixmap(AvatarQS::person(q(me), q(p.nickname()), 80));
    auto* names = new QVBoxLayout();
    auto* nick = label(q(p.nickname()), "PanelTitle", head);
    nick->setStyleSheet("font-size: 24px;");
    names->addWidget(nick);
    names->addWidget(label(q(s.name() + " · " + s.accountLabel() + " " + me), "SubTitle", head));
    names->addWidget(label(p.signature().empty() ? QString::fromUtf8("还没有个性签名") : q(p.signature()), "", head));
    top->addWidget(avatar);
    top->addLayout(names, 1);
    inner->addLayout(top);
    auto* grid = new QGridLayout();
    grid->setHorizontalSpacing(18);
    grid->setVerticalSpacing(8);
    struct FieldQS { const char* key; const char* name; std::string value; };
    const FieldQS fields[] = {{"nickname", "昵称", p.nickname()},
                              {"birthday", "出生日期", p.birthday() + "（" + std::to_string(p.age()) + "岁）"},
                              {"location", "所在地", p.location()},
                              {"signature", "个性签名", p.signature()}};
    int row = 0;
    for (const FieldQS& f : fields) {
        grid->addWidget(label(QString::fromUtf8(f.name), "SubTitle", head), row, 0);
        grid->addWidget(label(f.value.empty() ? QString::fromUtf8("—") : q(f.value), "", head), row, 1);
        auto* edit = new QPushButton(QString::fromUtf8("修改"), head);
        edit->setObjectName("LinkButton");
        grid->addWidget(edit, row, 2);
        const std::string key = f.key;
        const QString title = QString::fromUtf8(f.name);
        const std::string current = key == "birthday" ? p.birthday() : f.value;
        connect(edit, &QPushButton::clicked, this, [this, key, title, current] {
            QMap<QString, QString> out;
            if (!FormDialogQS::ask(this, QString::fromUtf8("修改") + title, "",
                                   {{"v", title, key == "birthday" ? "YYYY-MM-DD" : "", q(current)}}, out))
                return;
            context_.show(context_.svc().updateProfile(context_.me(), key, out["v"].toStdString()));
            context_.refreshAll();
        });
        ++row;
    }
    grid->addWidget(label(QString::fromUtf8("号码申请"), "SubTitle", head), row, 0);
    grid->addWidget(label(QString::fromUtf8("%1，T龄 %2 年").arg(q(p.registeredOn())).arg(p.tAge()), "", head), row, 1);
    grid->setColumnStretch(1, 1);
    inner->addLayout(grid);
    column->addWidget(head);

    // —— 关联账号 ——
    QWidget* links = makeCard(body_, inner, QString::fromUtf8("我在其他服务的账号"));
    links->setFixedWidth(width);
    inner->addWidget(label(QString::fromUtf8("由各服务自己保存的绑定关系汇总而来（QQ 与微博共用号码，微信可绑定 QQ，微商可绑定微信）。"
                                             "在任一服务登录后，这些服务都会出现在服务选择页的“一键进入”中。"),
                           "SubTitle", links));
    PlatformQS& platform = context_.client->platform();
    const auto linked = platform.linkedAccounts(context_.client->me(context_.service));
    if (linked.empty()) inner->addWidget(label(QString::fromUtf8("暂无关联账号"), "", links));
    for (const AccountRefQS& ref : linked) {
        ServiceBaseQS* other = platform.service(ref.service());
        if (other == nullptr) continue;
        auto* line = new QHBoxLayout();
        auto* logo = new QLabel(links);
        logo->setPixmap(AvatarQS::logo(QColor(q(other->accentColor())), q(other->logoText()), 28));
        line->addWidget(logo);
        line->addWidget(label(q(other->name() + "　" + other->accountLabel() + " " + ref.account()), "", links), 1);
        const bool direct = a->isLinkedTo(ref);
        if (direct) {
            auto* unbind = new QPushButton(QString::fromUtf8("解除绑定"), links);
            unbind->setObjectName("LinkButton");
            line->addWidget(unbind);
            connect(unbind, &QPushButton::clicked, this, [this, ref] {
                if (!FormDialogQS::confirm(this, QString::fromUtf8("解除绑定"), QString::fromUtf8("解除后两个账号各自独立，数据不受影响。"))) return;
                context_.show(context_.svc().unbindAccount(context_.me(), ref));
                context_.refreshAll();
            });
        }
        inner->addLayout(line);
    }
    const auto bindable = s.bindableServices();
    if (!bindable.empty()) {
        auto* bind = new QPushButton(IconQS::icon("link", ThemeQS::dark(context_.accent), 16), QString::fromUtf8("绑定其他服务账号"), links);
        bind->setObjectName("SoftButton");
        auto* br = new QHBoxLayout();
        br->addWidget(bind);
        br->addStretch(1);
        inner->addLayout(br);
        connect(bind, &QPushButton::clicked, this, [this, bindable] {
            QStringList names;
            for (const auto& id : bindable) names << q(context_.client->platform().service(id)->name());
            QMap<QString, QString> out;
            if (!FormDialogQS::ask(this, QString::fromUtf8("绑定账号"),
                                   QString::fromUtf8("账号与密码由对方服务核验，本服务只保存绑定关系。"),
                                   {{"svc", QString::fromUtf8("服务"), "", "", false, names},
                                    {"account", QString::fromUtf8("对方账号"), ""},
                                    {"password", QString::fromUtf8("对方密码"), "", "", true}},
                                   out, QString::fromUtf8("绑定")))
                return;
            const std::string target = bindable[static_cast<std::size_t>(names.indexOf(out["svc"]))];
            context_.show(context_.svc().bindAccount(context_.me(), AccountRefQS(target, out["account"].toStdString()),
                                                     out["password"].toStdString()));
            context_.refreshAll();
        });
    }
    column->addWidget(links);

    // —— 聊天与平台 ——
    QWidget* platformCard = makeCard(body_, inner, QString::fromUtf8("平台与网络"));
    platformCard->setFixedWidth(width);
    const bool chat = s.supportsChat();
    inner->addWidget(label(chat ? (context_.client->chatReady(context_.service)
                                       ? QString::fromUtf8("聊天节点已上线：本机端口 %1（系统自动分配）。好友在线时自动连接，无需填写地址。")
                                             .arg(context_.client->chat().port())
                                       : QString::fromUtf8("聊天未就绪：该账号已在另一个窗口登录"))
                                : QString::fromUtf8("%1没有聊天功能（服务之间的差异）").arg(q(s.name())),
                           "", platformCard));
    auto* pr = new QHBoxLayout();
    auto* log = new QPushButton(QString::fromUtf8("平台日志"), platformCard);
    pr->addWidget(log);
    if (chat) {
        auto* lan = new QPushButton(QString::fromUtf8("连接另一台电脑"), platformCard);
        pr->addWidget(lan);
        connect(lan, &QPushButton::clicked, this, [this] {
            QMap<QString, QString> out;
            if (!FormDialogQS::ask(this, QString::fromUtf8("连接另一台电脑"),
                                   QString::fromUtf8("同一局域网内另一台电脑以 --lan 参数启动后，填写它的 IP 和端口（在对方“我”页面可见）。"),
                                   {{"host", QString::fromUtf8("IP 地址"), "192.168.1.23"}, {"port", QString::fromUtf8("端口"), ""}}, out,
                                   QString::fromUtf8("连接")))
                return;
            context_.show(context_.client->chat().connectTo(out["host"].toStdString(), static_cast<unsigned short>(out["port"].toInt())));
        });
    }
    pr->addStretch(1);
    inner->addLayout(pr);
    connect(log, &QPushButton::clicked, this, [this] {
        BusLogDialogQS dialog(context_.client->platform(), this);
        dialog.exec();
    });
    column->addWidget(platformCard);

    // —— 账号与安全 ——
    QWidget* security = makeCard(body_, inner, QString::fromUtf8("账号与安全"));
    security->setFixedWidth(width);
    auto* sr = new QHBoxLayout();
    auto* password = new QPushButton(QString::fromUtf8("修改密码"), security);
    auto* logout = new QPushButton(IconQS::icon("logout", ThemeQS::text(), 16), QString::fromUtf8("退出%1登录").arg(q(s.name())), security);
    auto* switchUser = new QPushButton(QString::fromUtf8("切换用户"), security);
    auto* remove = new QPushButton(QString::fromUtf8("注销%1账号").arg(q(s.name())), security);
    remove->setObjectName("DangerButton");
    sr->addWidget(password);
    sr->addWidget(logout);
    sr->addWidget(switchUser);
    sr->addStretch(1);
    sr->addWidget(remove);
    inner->addLayout(sr);
    inner->addWidget(label(QString::fromUtf8("退出登录只结束本服务，其他服务保持登录；切换用户会退出全部服务。注销只删除本服务的账号数据，其他服务不受影响。"),
                           "SubTitle", security));
    column->addWidget(security);

    connect(password, &QPushButton::clicked, this, [this] {
        QMap<QString, QString> out;
        if (!FormDialogQS::ask(this, QString::fromUtf8("修改密码"), "",
                               {{"old", QString::fromUtf8("原密码"), "", "", true}, {"new", QString::fromUtf8("新密码"), QString::fromUtf8("至少6位"), "", true}},
                               out))
            return;
        context_.show(context_.svc().changePassword(context_.me(), out["old"].toStdString(), out["new"].toStdString()));
    });
    connect(logout, &QPushButton::clicked, this, [this] { context_.logoutService(); });
    connect(switchUser, &QPushButton::clicked, this, [this] { context_.switchUser(); });
    connect(remove, &QPushButton::clicked, this, [this] {
        QMap<QString, QString> out;
        if (!FormDialogQS::ask(this, QString::fromUtf8("注销账号"),
                               QString::fromUtf8("将删除你在%1的账号、好友关系和群成员身份，此操作不可恢复。其他服务的账号不受影响。")
                                   .arg(q(context_.svc().name())),
                               {{"password", QString::fromUtf8("输入密码确认"), "", "", true}}, out, QString::fromUtf8("确认注销")))
            return;
        const std::string me = context_.me();
        const ResultQS r = context_.svc().deactivate(me, out["password"].toStdString());
        context_.show(r);
        if (r) context_.logoutService();
    });
}

QS_REGISTER_GUI_FEATURE(ProfilePageQS, "me")

}  // namespace qs
