#include "ContactsPageQS.h"

#include "AvatarQS.h"
#include "LayoutUtilQS.h"
#include "FormDialogQS.h"
#include "IconQS.h"
#include "ListRowQS.h"
#include "ThemeQS.h"

#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QScrollArea>

#include <map>

namespace qs {

namespace {
const int kKeyRole = Qt::UserRole + 1;

QString q(const std::string& s) { return QString::fromStdString(s); }

QLabel* label(const QString& text, const char* objectName, QWidget* parent) {
    auto* l = new QLabel(text, parent);
    l->setObjectName(objectName);
    l->setWordWrap(true);
    return l;
}
}  // namespace

ContactsPageQS::ContactsPageQS(ShellContextQS& context, QWidget* parent) : FeaturePageQS(context, parent) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* panel = new QWidget(this);
    panel->setObjectName("ListPanel");
    panel->setAttribute(Qt::WA_StyledBackground);
    panel->setFixedWidth(310);
    auto* p = new QVBoxLayout(panel);
    p->setContentsMargins(10, 16, 10, 8);
    p->setSpacing(10);
    auto* head = new QHBoxLayout();
    head->setContentsMargins(8, 0, 2, 0);
    QString title = q(context_.svc().friendCapability()->relationName());
    for (const auto& f : context_.svc().features()) {
        if (f.kind == "contacts") title = q(f.title);
    }
    auto* heading = new QLabel(title, panel);
    heading->setObjectName("PanelTitle");
    auto* add = new QPushButton(panel);
    add->setObjectName("IconButton");
    add->setIcon(IconQS::icon("user-add", ThemeQS::text(), 20));
    add->setToolTip(q(context_.svc().friendCapability()->addActionName()));
    add->setFixedSize(36, 36);
    head->addWidget(heading);
    head->addStretch(1);
    head->addWidget(add);
    p->addLayout(head);
    search_ = new QLineEdit(panel);
    search_->setObjectName("SearchBox");
    search_->setPlaceholderText(QString::fromUtf8("搜索昵称、号码、备注"));
    search_->setClearButtonEnabled(true);
    search_->addAction(IconQS::icon("search", ThemeQS::subtext(), 16), QLineEdit::LeadingPosition);
    search_->setStyleSheet("padding-left: 6px;");
    p->addWidget(search_);
    list_ = new QListWidget(panel);
    list_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    p->addWidget(list_, 1);
    layout->addWidget(panel);

    auto* scroll = new QScrollArea(this);
    scroll->setObjectName("ContentPanel");
    scroll->setWidgetResizable(true);
    detail_ = new QWidget(scroll);
    detail_->setObjectName("ContentPanel");
    detail_->setAttribute(Qt::WA_StyledBackground);
    detailLayout_ = new QVBoxLayout(detail_);
    detailLayout_->setContentsMargins(40, 32, 40, 32);
    detailLayout_->setSpacing(16);
    scroll->setWidget(detail_);
    layout->addWidget(scroll, 1);

    connect(search_, &QLineEdit::textChanged, this, [this] { rebuildList(); });
    connect(add, &QPushButton::clicked, this, [this] {
        selected_ = "@add";
        list_->clearSelection();
        showAddPanel();
    });
    connect(list_, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        const std::string key = item->data(kKeyRole).toString().toStdString();
        if (key.empty()) return;
        selected_ = key;
        if (key == "@requests") showRequests();
        else showFriend(key);
        updateSelection();
    });
    connect(list_, &QListWidget::currentRowChanged, this, [this] { updateSelection(); });
    rebuildList();
    showAddPanel();
    selected_ = "@add";
}

int ContactsPageQS::badgeCount() const {
    const FriendCapabilityQS* f = context_.svc().friendCapability();
    return f ? static_cast<int>(f->incomingRequests(context_.me()).size()) : 0;
}

void ContactsPageQS::updateSelection() {
    // 行内文字颜色与列表的选中背景必须同步，否则选中行会出现"白字白底"
    list_->blockSignals(true);
    list_->clearSelection();
    for (int i = 0; i < list_->count(); ++i) {
        const bool match = list_->item(i)->data(kKeyRole).toString().toStdString() == selected_;
        if (match) list_->setCurrentItem(list_->item(i));
        if (auto* row = dynamic_cast<ListRowQS*>(list_->itemWidget(list_->item(i)))) row->setSelected(match);
    }
    list_->blockSignals(false);
}

void ContactsPageQS::rebuildList() {
    list_->clear();
    ServiceBaseQS& s = context_.svc();
    FriendCapabilityQS* f = s.friendCapability();
    const std::string me = context_.me();
    const QString keyword = search_->text().trimmed();

    // "新的朋友 / 新粉丝"入口
    if (keyword.isEmpty()) {
        auto* item = new QListWidgetItem(list_);
        item->setSizeHint(QSize(0, ListRowQS::rowHeight()));
        item->setData(kKeyRole, "@requests");
        auto* row = new ListRowQS(list_);
        row->setAvatar(AvatarQS::logo(context_.accent, "+", 46, 23));
        row->setTitle(f->requiresApproval() ? QString::fromUtf8("新的朋友") : QString::fromUtf8("新粉丝"));
        const auto incoming = f->incomingRequests(me);
        row->setSubtitle(incoming.empty() ? QString::fromUtf8("暂无新的申请")
                                          : QString::fromUtf8("%1 条待处理").arg(incoming.size()));
        row->setBadge(static_cast<int>(incoming.size()));
        list_->setItemWidget(item, row);
    }

    // 按分组排列好友
    std::map<std::string, std::vector<FriendViewQS>> byTag;
    for (const auto& v : f->friendsOf(me)) {
        const QString hay = q(v.account + " " + v.nickname + " " + v.remark + " " + v.tag);
        if (!keyword.isEmpty() && !hay.contains(keyword, Qt::CaseInsensitive)) continue;
        byTag[v.tag.empty() ? "~未分组" : v.tag].push_back(v);
    }
    int total = 0;
    for (const auto& group : byTag) {
        const QString name = group.first == "~未分组" ? QString::fromUtf8("未分组") : q(group.first);
        auto* section = new QListWidgetItem(QString::fromUtf8("  %1 · %2").arg(name).arg(group.second.size()), list_);
        section->setFlags(Qt::NoItemFlags);
        section->setForeground(ThemeQS::subtext());
        for (const auto& v : group.second) {
            ++total;
            auto* item = new QListWidgetItem(list_);
            item->setSizeHint(QSize(0, ListRowQS::rowHeight()));
            item->setData(kKeyRole, q(v.account));
            auto* row = new ListRowQS(list_);
            row->setAvatar(AvatarQS::person(q(v.account), q(v.nickname), 46));
            row->setTitle(q(v.displayName()));
            QString sub = q(s.accountLabel() + " " + v.account);
            if (!v.remark.empty()) sub += QString::fromUtf8(" · 昵称 ") + q(v.nickname);
            row->setSubtitle(sub);
            if (s.supportsChat()) row->setOnline(context_.client->isOnline(context_.service, v.account));
            list_->setItemWidget(item, row);
        }
    }
    if (total == 0) {
        auto* item = new QListWidgetItem(keyword.isEmpty() ? QString::fromUtf8("  还没有") + q(f->relationName())
                                                           : QString::fromUtf8("  没有匹配的") + q(f->relationName()),
                                         list_);
        item->setFlags(Qt::NoItemFlags);
        item->setForeground(ThemeQS::subtext());
    }
    updateSelection();
}

void ContactsPageQS::clearDetail() {
    LayoutUtilQS::clear(detailLayout_);
}

QWidget* ContactsPageQS::card(QWidget* parent, QVBoxLayout*& inner) {
    auto* c = new QWidget(parent);
    c->setObjectName("Card");
    c->setAttribute(Qt::WA_StyledBackground);
    inner = new QVBoxLayout(c);
    inner->setContentsMargins(24, 20, 24, 20);
    inner->setSpacing(10);
    return c;
}

void ContactsPageQS::showFriend(const std::string& account) {
    clearDetail();
    ServiceBaseQS& s = context_.svc();
    FriendCapabilityQS* f = s.friendCapability();
    FriendViewQS view;
    bool found = false;
    for (const auto& v : f->friendsOf(context_.me())) {
        if (v.account == account) {
            view = v;
            found = true;
        }
    }
    if (!found) {
        showAddPanel();
        return;
    }
    const ServiceAccountQS* a = s.account(account);
    QVBoxLayout* inner = nullptr;
    QWidget* c = card(detail_, inner);
    auto* top = new QHBoxLayout();
    top->setSpacing(18);
    auto* avatar = new QLabel(c);
    avatar->setPixmap(AvatarQS::person(q(account), q(view.nickname), 72));
    auto* names = new QVBoxLayout();
    names->setSpacing(4);
    auto* name = label(q(view.displayName()), "PanelTitle", c);
    name->setStyleSheet("font-size: 22px;");
    names->addWidget(name);
    names->addWidget(label(q(s.accountLabel() + "：" + account + (view.remark.empty() ? "" : "　昵称：" + view.nickname)), "SubTitle", c));
    if (s.supportsChat()) {
        const bool online = context_.client->isOnline(context_.service, account);
        auto* status = label(online ? QString::fromUtf8("● 在线") : QString::fromUtf8("○ 离线"), "SubTitle", c);
        if (online) status->setStyleSheet("color:#22A35A;");
        names->addWidget(status);
    }
    names->addStretch(1);
    top->addWidget(avatar);
    top->addLayout(names, 1);
    inner->addLayout(top);
    auto* divider = new QWidget(c);
    divider->setObjectName("Divider");
    inner->addWidget(divider);

    auto* grid = new QGridLayout();
    grid->setHorizontalSpacing(24);
    grid->setVerticalSpacing(10);
    int row = 0;
    const auto addRow = [&](const QString& k, const QString& v) {
        auto* key = label(k, "SubTitle", c);
        grid->addWidget(key, row, 0);
        grid->addWidget(label(v.isEmpty() ? QString::fromUtf8("—") : v, "", c), row, 1);
        ++row;
    };
    addRow(QString::fromUtf8("备注"), q(view.remark));
    addRow(QString::fromUtf8("分组"), q(view.tag));
    addRow(QString::fromUtf8("关系"), q(view.relation) + QString::fromUtf8("（自 %1 起）").arg(q(view.since)));
    if (a) {
        addRow(QString::fromUtf8("所在地"), q(a->profile().location()));
        addRow(QString::fromUtf8("T龄"), QString::fromUtf8("%1 年").arg(a->profile().tAge()));
        addRow(QString::fromUtf8("个性签名"), q(a->profile().signature()));
    }
    grid->setColumnStretch(1, 1);
    inner->addLayout(grid);

    auto* buttons = new QHBoxLayout();
    buttons->setSpacing(10);
    if (s.supportsChat()) {
        auto* chat = new QPushButton(IconQS::icon("chat", Qt::white, 18), QString::fromUtf8("发消息"), c);
        chat->setObjectName("PrimaryButton");
        connect(chat, &QPushButton::clicked, this, [this, account] { context_.openConversation("p:" + account); });
        buttons->addWidget(chat);
    }
    auto* edit = new QPushButton(QString::fromUtf8("备注与分组"), c);
    auto* remove = new QPushButton(f->requiresApproval() ? QString::fromUtf8("删除") + q(f->relationName())
                                                         : QString::fromUtf8("取消关注"),
                                   c);
    remove->setObjectName("DangerButton");
    buttons->addWidget(edit);
    buttons->addStretch(1);
    buttons->addWidget(remove);
    inner->addSpacing(6);
    inner->addLayout(buttons);
    detailLayout_->addWidget(c);
    detailLayout_->addStretch(1);

    connect(edit, &QPushButton::clicked, this, [this, view] {
        QMap<QString, QString> out;
        FormFieldQS tag{"tag", QString::fromUtf8("分组"), "", q(view.tag), false,
                        {QString::fromUtf8("同学"), QString::fromUtf8("朋友"), QString::fromUtf8("家人"),
                         QString::fromUtf8("老师"), QString::fromUtf8("同事"), QString()},
                        true};
        if (!FormDialogQS::ask(this, QString::fromUtf8("修改备注与分组"), q(view.nickname + "（" + view.account + "）"),
                               {{"remark", QString::fromUtf8("备注"), QString::fromUtf8("可留空"), q(view.remark)}, tag}, out))
            return;
        context_.show(context_.svc().friendCapability()->updateFriend(context_.me(), view.account, out["remark"].toStdString(),
                                                                       out["tag"].toStdString()));
        context_.refreshAll();
    });
    connect(remove, &QPushButton::clicked, this, [this, view] {
        if (!FormDialogQS::confirm(this, QString::fromUtf8("确认删除"),
                                   QString::fromUtf8("确定要删除 %1 吗？只影响当前服务，其他服务的关系不变。").arg(q(view.displayName())),
                                   QString::fromUtf8("删除"), true))
            return;
        context_.show(context_.svc().friendCapability()->removeFriend(context_.me(), view.account));
        selected_ = "@add";
        context_.refreshAll();
    });
}

void ContactsPageQS::showRequests() {
    clearDetail();
    ServiceBaseQS& s = context_.svc();
    FriendCapabilityQS* f = s.friendCapability();
    const std::string me = context_.me();
    detailLayout_->addWidget(label(f->requiresApproval() ? QString::fromUtf8("新的朋友") : QString::fromUtf8("新粉丝"), "PanelTitle", detail_));
    const auto incoming = f->incomingRequests(me);
    if (incoming.empty()) detailLayout_->addWidget(label(QString::fromUtf8("暂时没有待处理的申请"), "SubTitle", detail_));
    for (const auto& r : incoming) {
        QVBoxLayout* inner = nullptr;
        QWidget* c = card(detail_, inner);
        auto* row = new QHBoxLayout();
        row->setSpacing(14);
        auto* avatar = new QLabel(c);
        avatar->setPixmap(AvatarQS::person(q(r.from), q(s.nicknameOf(r.from)), 48));
        auto* text = new QVBoxLayout();
        text->addWidget(label(q(s.nicknameOf(r.from) + "（" + r.from + "）"), "RowTitle", c));
        text->addWidget(label(q(r.note.empty() ? "请求添加你为" + f->relationName() : r.note) + "　" + q(r.date), "SubTitle", c));
        auto* accept = new QPushButton(f->requiresApproval() ? QString::fromUtf8("同意") : QString::fromUtf8("回关"), c);
        accept->setObjectName("PrimaryButton");
        auto* reject = new QPushButton(f->requiresApproval() ? QString::fromUtf8("拒绝") : QString::fromUtf8("忽略"), c);
        row->addWidget(avatar);
        row->addLayout(text, 1);
        row->addWidget(reject);
        row->addWidget(accept);
        inner->addLayout(row);
        detailLayout_->addWidget(c);
        const std::string id = r.id;
        connect(accept, &QPushButton::clicked, this, [this, id] {
            context_.show(context_.svc().friendCapability()->answerRequest(context_.me(), id, true));
            context_.refreshAll();
        });
        connect(reject, &QPushButton::clicked, this, [this, id] {
            context_.show(context_.svc().friendCapability()->answerRequest(context_.me(), id, false));
            context_.refreshAll();
        });
    }
    const auto outgoing = f->outgoingRequests(me);
    if (!outgoing.empty()) {
        detailLayout_->addWidget(label(QString::fromUtf8("我发出的（等待对方处理）"), "SectionLabel", detail_));
        for (const auto& r : outgoing) {
            detailLayout_->addWidget(label(q("· " + s.nicknameOf(r.to) + "（" + r.to + "）  " + r.date + "  " + r.note), "", detail_));
        }
    }
    detailLayout_->addStretch(1);
}

void ContactsPageQS::showAddPanel() {
    clearDetail();
    ServiceBaseQS& s = context_.svc();
    FriendCapabilityQS* f = s.friendCapability();
    PlatformQS& platform = context_.client->platform();
    detailLayout_->addWidget(label(q(f->addActionName()), "PanelTitle", detail_));

    // 1) 按号码添加
    QVBoxLayout* inner = nullptr;
    QWidget* byId = card(detail_, inner);
    inner->addWidget(label(QString::fromUtf8("按%1添加").arg(q(s.accountLabel())), "RowTitle", byId));
    auto* row = new QHBoxLayout();
    auto* account = new QLineEdit(byId);
    account->setPlaceholderText(QString::fromUtf8("输入对方%1").arg(q(s.accountLabel())));
    auto* note = new QLineEdit(byId);
    note->setPlaceholderText(f->requiresApproval() ? QString::fromUtf8("验证消息，如：我是丘山") : QString::fromUtf8("（关注无需验证）"));
    note->setEnabled(f->requiresApproval());
    auto* send = new QPushButton(q(f->addActionName()), byId);
    send->setObjectName("PrimaryButton");
    row->addWidget(account, 2);
    row->addWidget(note, 3);
    row->addWidget(send);
    inner->addLayout(row);
    detailLayout_->addWidget(byId);
    connect(send, &QPushButton::clicked, this, [this, account, note] {
        if (account->text().trimmed().isEmpty()) return;
        context_.show(context_.svc().friendCapability()->addFriend(context_.me(), account->text().trimmed().toStdString(),
                                                                   note->text().trimmed().toStdString()));
        context_.refreshAll();
    });

    // 2) 从其他服务推荐（题目：微信可以添加 QQ 推荐好友；可依据另一服务的好友添加）
    QWidget* rec = card(detail_, inner);
    auto* recHead = new QHBoxLayout();
    recHead->addWidget(label(QString::fromUtf8("从其他服务的好友中推荐"), "RowTitle", rec), 1);
    auto* source = new QComboBox(rec);
    for (const auto& other : platform.services()) {
        if (other->id() != context_.service && other->friendCapability() != nullptr) {
            source->addItem(q(other->name()), q(other->id()));
        }
    }
    auto* find = new QPushButton(QString::fromUtf8("查找"), rec);
    find->setObjectName("SoftButton");
    auto* common = new QPushButton(QString::fromUtf8("共同好友"), rec);
    recHead->addWidget(source);
    recHead->addWidget(find);
    recHead->addWidget(common);
    inner->addLayout(recHead);
    inner->addWidget(label(QString::fromUtf8("平台经服务总线向来源服务查询你的好友，再找出他们在本服务的账号；服务之间不直接访问对方的数据。"),
                           "SubTitle", rec));
    auto* results = new QVBoxLayout();
    results->setSpacing(8);
    inner->addLayout(results);
    detailLayout_->addWidget(rec);
    detailLayout_->addStretch(1);

    const auto clearResults = [results] {
        LayoutUtilQS::clear(results);
    };
    connect(find, &QPushButton::clicked, this, [this, source, results, rec, clearResults] {
        clearResults();
        std::string hint;
        const std::string from = source->currentData().toString().toStdString();
        const auto list = context_.client->platform().recommendFriends(context_.service, context_.me(), from, hint);
        results->addWidget(label(q(hint), "SubTitle", rec));
        for (const auto& c : list) {
            auto* line = new QHBoxLayout();
            auto* avatar = new QLabel(rec);
            avatar->setPixmap(AvatarQS::person(q(c.account), q(c.nickname), 36));
            line->addWidget(avatar);
            auto* text = new QVBoxLayout();
            text->setSpacing(0);
            text->addWidget(label(q(c.nickname + "（" + c.account + "）"), "RowTitle", rec));
            text->addWidget(label(q(c.reason), "SubTitle", rec));
            line->addLayout(text, 1);
            auto* add = new QPushButton(q(context_.svc().friendCapability()->addActionName()), rec);
            add->setObjectName("SoftButton");
            line->addWidget(add);
            results->addLayout(line);
            const std::string target = c.account;
            const std::string fromName = context_.client->platform().service(from)->name();
            connect(add, &QPushButton::clicked, this, [this, target, fromName, add] {
                const ResultQS r = context_.svc().friendCapability()->addFriend(
                    context_.me(), target, "我是" + context_.svc().nicknameOf(context_.me()) + "，来自" + fromName + "好友推荐");
                context_.show(r);
                if (r) {
                    add->setEnabled(false);
                    add->setText(QString::fromUtf8("已发送"));
                }
            });
        }
    });
    connect(common, &QPushButton::clicked, this, [this, source, results, rec, clearResults] {
        clearResults();
        std::string hint;
        const std::string other = source->currentData().toString().toStdString();
        const auto list = context_.client->platform().commonFriends(context_.client->me(context_.service), other, hint);
        results->addWidget(label(q(hint), "SubTitle", rec));
        for (const auto& x : list) {
            results->addWidget(label(q("· " + x.nickname + "　" + context_.svc().name() + " " + x.first.account() + "　/　" +
                                       context_.client->platform().service(other)->name() + " " + x.second.account()),
                                     "", rec));
        }
    });
}

void ContactsPageQS::refresh() {
    rebuildList();
    if (selected_ == "@requests") showRequests();
    else if (selected_ != "@add" && !selected_.empty()) showFriend(selected_);
}

void ContactsPageQS::activate(const std::string& account) {
    search_->clear();
    selected_ = account;
    rebuildList();
    showFriend(account);
}

QS_REGISTER_GUI_FEATURE(ContactsPageQS, "contacts")

}  // namespace qs
