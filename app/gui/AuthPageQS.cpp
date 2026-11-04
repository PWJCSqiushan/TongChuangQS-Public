#include "AuthPageQS.h"

#include "AvatarQS.h"
#include "IconQS.h"
#include "ThemeQS.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QScrollArea>

namespace qs {

namespace {
QString q(const std::string& s) { return QString::fromStdString(s); }
}

AuthPageQS::AuthPageQS(ClientQS& client, std::function<void(const std::string&, const QString&)> onLoggedIn, std::function<void()> onBack,
                       QWidget* parent)
    : QWidget(parent), client_(client), onLoggedIn_(std::move(onLoggedIn)), onBack_(std::move(onBack)) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
}

void AuthPageQS::setService(const std::string& service) {
    service_ = service;
    build();
}

void AuthPageQS::setNotice(const QString& text) {
    if (message_) {
        message_->setText(text);
        message_->setStyleSheet("color:#E5484D;");
    }
}

void AuthPageQS::build() {
    if (content_) {
        content_->hide();
        content_->deleteLater();
    }
    fields_.clear();
    linked_.clear();
    ServiceBaseQS& s = *client_.platform().service(service_);
    const QColor accent(q(s.accentColor()));
    content_ = new QWidget(this);
    content_->setStyleSheet(ThemeQS::styleSheet(accent));
    layout()->addWidget(content_);
    auto* row = new QHBoxLayout(content_);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    // —— 左侧品牌区（主题色渐变）——
    auto* brand = new QWidget(content_);
    brand->setObjectName("Brand");
    brand->setAttribute(Qt::WA_StyledBackground);
    brand->setFixedWidth(380);
    brand->setStyleSheet(QString("#Brand{background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 %1,stop:1 %2);}"
                                 "#Brand QLabel{background:transparent;color:white;}")
                             .arg(accent.lighter(112).name(), ThemeQS::dark(accent).name()));
    auto* b = new QVBoxLayout(brand);
    b->setContentsMargins(36, 28, 36, 36);
    auto* back = new QPushButton(IconQS::icon("back", Qt::white, 16), QString::fromUtf8("返回服务选择"), brand);
    back->setStyleSheet("QPushButton{background:rgba(255,255,255,0.16);color:white;border:none;border-radius:10px;padding:7px 14px;}"
                        "QPushButton:hover{background:rgba(255,255,255,0.28);}");
    b->addWidget(back, 0, Qt::AlignLeft);
    b->addStretch(2);
    auto* logo = new QLabel(brand);
    logo->setPixmap(AvatarQS::logo(accent.lighter(125), q(s.logoText()), 84, 24));
    b->addWidget(logo);
    b->addSpacing(14);
    auto* name = new QLabel(q(s.name()), brand);
    name->setStyleSheet("font-size:34px;font-weight:700;");
    b->addWidget(name);
    auto* slogan = new QLabel(q(s.slogan()), brand);
    slogan->setWordWrap(true);
    slogan->setStyleSheet("font-size:15px;");
    b->addWidget(slogan);
    b->addSpacing(18);
    QStringList featureNames;
    for (const auto& f : s.features()) featureNames << q(f.title);
    auto* features = new QLabel(QString::fromUtf8("本服务提供：") + featureNames.join(QString::fromUtf8(" · ")), brand);
    features->setWordWrap(true);
    features->setStyleSheet("font-size:13px;color:rgba(255,255,255,0.88);");
    b->addWidget(features);
    auto* rule = new QLabel(q(s.idRuleText()), brand);
    rule->setWordWrap(true);
    rule->setStyleSheet("font-size:12px;color:rgba(255,255,255,0.78);");
    b->addWidget(rule);
    b->addStretch(3);
    row->addWidget(brand);
    connect(back, &QPushButton::clicked, this, [this] { onBack_(); });

    // —— 右侧登录/注册卡片 ——
    auto* right = new QWidget(content_);
    right->setObjectName("ContentPanel");
    right->setAttribute(Qt::WA_StyledBackground);
    auto* r = new QVBoxLayout(right);
    r->setContentsMargins(40, 30, 40, 30);
    auto* cardRow = new QHBoxLayout();
    cardRow->addStretch(1);
    auto* card = new QWidget(right);
    card->setObjectName("Card");
    card->setAttribute(Qt::WA_StyledBackground);
    card->setFixedWidth(420);
    auto* c = new QVBoxLayout(card);
    c->setContentsMargins(30, 24, 30, 26);
    c->setSpacing(10);
    cardRow->addWidget(card);
    cardRow->addStretch(1);
    r->addStretch(1);
    r->addLayout(cardRow);
    r->addStretch(1);
    row->addWidget(right, 1);

    const LoginSessionQS& session = client_.platform().session();
    if (session.isPending(service_)) {
        auto* banner = new QWidget(card);
        banner->setObjectName("AccentCard");
        banner->setAttribute(Qt::WA_StyledBackground);
        auto* bl = new QVBoxLayout(banner);
        bl->setContentsMargins(16, 12, 16, 12);
        QStringList via;
        for (const auto& ref : session.loggedInAccounts()) via << q(client_.platform().service(ref.service())->name() + " " + ref.account());
        auto* text = new QLabel(QString::fromUtf8("你已通过 %1 登录，系统发现你开通了%2账号 %3。")
                                    .arg(via.join(QString::fromUtf8("、")), q(s.name()), q(session.accountOf(service_))),
                                banner);
        text->setWordWrap(true);
        auto* go = new QPushButton(QString::fromUtf8("确认，一键进入"), banner);
        go->setObjectName("PrimaryButton");
        bl->addWidget(text);
        bl->addWidget(go);
        c->addWidget(banner);
        connect(go, &QPushButton::clicked, this, [this] { doConfirmPending(); });
    }

    auto* tabs = new QTabBar(card);
    tabs->addTab(QString::fromUtf8("登录"));
    tabs->addTab(QString::fromUtf8("注册新账号"));
    tabs->setExpanding(false);
    tabs->setDrawBase(false);
    c->addWidget(tabs);
    auto* stack = new QStackedWidget(card);
    c->addWidget(stack);
    message_ = new QLabel(card);
    message_->setWordWrap(true);
    c->addWidget(message_);

    // 登录
    auto* login = new QWidget(stack);
    auto* l = new QVBoxLayout(login);
    l->setContentsMargins(0, 8, 0, 0);
    l->setSpacing(8);
    auto* aLabel = new QLabel(q(s.accountLabel()), login);
    aLabel->setStyleSheet("font-weight:600;font-size:12px;color:#4A505A;");
    account_ = new QLineEdit(login);
    account_->setPlaceholderText(QString::fromUtf8("请输入") + q(s.accountLabel()));
    auto* pLabel = new QLabel(QString::fromUtf8("密码"), login);
    pLabel->setStyleSheet("font-weight:600;font-size:12px;color:#4A505A;");
    password_ = new QLineEdit(login);
    password_->setEchoMode(QLineEdit::Password);
    password_->setPlaceholderText(QString::fromUtf8("请输入密码"));
    auto* go = new QPushButton(QString::fromUtf8("登 录"), login);
    go->setObjectName("PrimaryButton");
    go->setMinimumHeight(42);
    l->addWidget(aLabel);
    l->addWidget(account_);
    l->addWidget(pLabel);
    l->addWidget(password_);
    l->addSpacing(6);
    l->addWidget(go);
    // 演示账号快捷填写
    auto* demoLabel = new QLabel(QString::fromUtf8("演示账号（密码均为 123456）："), login);
    demoLabel->setObjectName("SubTitle");
    l->addSpacing(6);
    l->addWidget(demoLabel);
    auto* chips = new QGridLayout();
    chips->setHorizontalSpacing(8);
    chips->setVerticalSpacing(8);
    int shown = 0;
    for (const ServiceAccountQS* a : s.accounts().all()) {
        if (shown >= 6) break;
        auto* chip = new QPushButton(q(a->profile().nickname() + " " + a->id()), login);
        chip->setObjectName("ChipButton");
        const QString id = q(a->id());
        connect(chip, &QPushButton::clicked, this, [this, id] {
            account_->setText(id);
            password_->setText("123456");
            password_->setFocus();
        });
        chip->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        chips->addWidget(chip, shown / 3, shown % 3);
        ++shown;
    }
    l->addLayout(chips);
    l->addStretch(1);  // 登录页比注册页短：把内容顶到上方，避免被拉开
    stack->addWidget(login);
    connect(go, &QPushButton::clicked, this, [this] { doLogin(); });
    connect(password_, &QLineEdit::returnPressed, this, [this] { doLogin(); });
    connect(account_, &QLineEdit::returnPressed, this, [this] { password_->setFocus(); });

    // 注册：字段由服务决定
    auto* reg = new QWidget(stack);
    auto* rl = new QVBoxLayout(reg);
    rl->setContentsMargins(0, 8, 0, 0);
    rl->setSpacing(6);
    for (const RegistrationFieldQS& f : s.registrationFields()) {
        auto* fl = new QLabel(q(f.label) + (f.optional ? QString::fromUtf8("（可选）") : QString()), reg);
        fl->setStyleSheet("font-weight:600;font-size:12px;color:#4A505A;");
        auto* edit = new QLineEdit(reg);
        edit->setPlaceholderText(q(f.placeholder));
        if (f.secret) edit->setEchoMode(QLineEdit::Password);
        rl->addWidget(fl);
        rl->addWidget(edit);
        fields_.emplace_back(f.key, edit);
    }
    const auto targets = client_.platform().linkedRegistrationTargets(service_);
    if (!targets.empty()) {
        auto* linkTitle = new QLabel(QString::fromUtf8("同时开通（由对应服务各自注册、分别保存，可单独注销）"), reg);
        linkTitle->setObjectName("SubTitle");
        linkTitle->setWordWrap(true);
        rl->addSpacing(4);
        rl->addWidget(linkTitle);
        for (const std::string& t : targets) {
            auto* box = new QCheckBox(q(client_.platform().service(t)->linkedRegistrationHint()), reg);
            rl->addWidget(box);
            linked_.emplace_back(t, box);
        }
    }
    auto* create = new QPushButton(QString::fromUtf8("注 册"), reg);
    create->setObjectName("PrimaryButton");
    create->setMinimumHeight(42);
    rl->addSpacing(6);
    rl->addWidget(create);
    stack->addWidget(reg);
    connect(create, &QPushButton::clicked, this, [this] { doRegister(); });
    // 卡片高度跟随当前页：非当前页的尺寸策略设为 Ignored，堆叠控件就只按当前页计算高度
    const auto fitStack = [stack](int i) {
        for (int k = 0; k < stack->count(); ++k) {
            stack->widget(k)->setSizePolicy(QSizePolicy::Preferred, k == i ? QSizePolicy::Preferred : QSizePolicy::Ignored);
        }
        stack->adjustSize();
    };
    fitStack(0);
    connect(tabs, &QTabBar::currentChanged, this, [this, stack, fitStack](int i) {
        fitStack(i);
        stack->setCurrentIndex(i);
        message_->clear();
    });
    account_->setFocus();
}

void AuthPageQS::doLogin() {
    const ResultQS r = client_.login(service_, account_->text().trimmed().toStdString(), password_->text().toStdString());
    if (!r) {
        message_->setStyleSheet("color:#E5484D;");
        message_->setText(q(r.message()));
        return;
    }
    onLoggedIn_(service_, q(r.message()));
}

void AuthPageQS::doConfirmPending() {
    const ResultQS r = client_.confirmPending(service_);
    if (!r) {
        message_->setStyleSheet("color:#E5484D;");
        message_->setText(q(r.message()));
        return;
    }
    onLoggedIn_(service_, q(r.message()));
}

void AuthPageQS::doRegister() {
    ServiceBaseQS& s = *client_.platform().service(service_);
    FormQS form;
    for (const auto& f : fields_) form[f.first] = f.second->text().trimmed().toStdString();
    std::string id;
    const ResultQS r = s.registerAccount(form, id);
    if (!r) {
        message_->setStyleSheet("color:#E5484D;");
        message_->setText(q(r.message()));
        return;
    }
    QStringList lines{q(r.message())};
    std::vector<std::string> chosen;
    for (const auto& l : linked_) {
        if (l.second->isChecked()) chosen.push_back(l.first);
    }
    for (const auto& result : client_.platform().registerLinked(AccountRefQS(service_, id), form["password"], chosen)) {
        lines << q((result.second ? "✓ " : "✗ ") + result.second.message());
    }
    const ResultQS login = client_.login(service_, id, form["password"]);
    message_->setStyleSheet("color:#16803C;");
    message_->setText(lines.join("\n"));
    if (login) onLoggedIn_(service_, lines.join(QString::fromUtf8("；")));
}

}  // namespace qs
