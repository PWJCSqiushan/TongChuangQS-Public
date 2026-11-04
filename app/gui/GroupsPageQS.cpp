#include "GroupsPageQS.h"

#include "AvatarQS.h"
#include "LayoutUtilQS.h"
#include "FormDialogQS.h"
#include "GroupPolicyRegistryQS.h"
#include "IconQS.h"
#include "ListRowQS.h"
#include "ThemeQS.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <QScrollArea>

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
QWidget* makeCard(QWidget* parent, QVBoxLayout*& inner, const char* name = "Card") {
    auto* c = new QWidget(parent);
    c->setObjectName(name);
    c->setAttribute(Qt::WA_StyledBackground);
    inner = new QVBoxLayout(c);
    inner->setContentsMargins(22, 18, 22, 18);
    inner->setSpacing(10);
    return c;
}
QPixmap groupAvatar(ServiceBaseQS& s, const GroupQS& g, int size) {
    QStringList seeds, names;
    for (const auto& m : g.members()) {
        seeds << q(m.account());
        names << q(s.nicknameOf(m.account()));
    }
    return AvatarQS::group(seeds, names, size);
}
}  // namespace

GroupsPageQS::GroupsPageQS(ShellContextQS& context, QWidget* parent) : FeaturePageQS(context, parent) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    auto* panel = new QWidget(this);
    panel->setObjectName("ListPanel");
    panel->setAttribute(Qt::WA_StyledBackground);
    panel->setFixedWidth(310);
    auto* p = new QVBoxLayout(panel);
    p->setContentsMargins(10, 16, 10, 8);
    p->setSpacing(8);
    auto* head = new QHBoxLayout();
    head->setContentsMargins(8, 0, 2, 0);
    QString title = QString::fromUtf8("群聊");
    for (const auto& f : context_.svc().features()) {
        if (f.kind == "groups") title = q(f.title);
    }
    auto* heading = new QLabel(title, panel);
    heading->setObjectName("PanelTitle");
    auto* create = new QPushButton(panel);
    create->setObjectName("IconButton");
    create->setIcon(IconQS::icon("plus", ThemeQS::text(), 20));
    create->setToolTip(QString::fromUtf8("新建群"));
    create->setFixedSize(36, 36);
    head->addWidget(heading);
    head->addStretch(1);
    head->addWidget(create);
    p->addLayout(head);
    search_ = new QLineEdit(panel);
    search_->setObjectName("SearchBox");
    search_->setPlaceholderText(QString::fromUtf8("搜索群名或群号"));
    search_->setClearButtonEnabled(true);
    search_->addAction(IconQS::icon("search", ThemeQS::subtext(), 16), QLineEdit::LeadingPosition);
    search_->setStyleSheet("padding-left: 6px;");
    p->addWidget(search_);
    tabs_ = new QTabBar(panel);
    tabs_->addTab(QString::fromUtf8("我的群"));
    tabs_->addTab(QString::fromUtf8("群目录"));
    tabs_->setExpanding(true);
    tabs_->setDrawBase(false);
    p->addWidget(tabs_);
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
    detailLayout_->setContentsMargins(36, 28, 36, 28);
    detailLayout_->setSpacing(14);
    scroll->setWidget(detail_);
    layout->addWidget(scroll, 1);

    connect(create, &QPushButton::clicked, this, [this] { createGroup(); });
    connect(search_, &QLineEdit::textChanged, this, [this] { rebuildList(); });
    connect(tabs_, &QTabBar::currentChanged, this, [this] { rebuildList(); });
    connect(list_, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        const std::string id = item->data(kKeyRole).toString().toStdString();
        if (id.empty()) return;
        selected_ = id;
        updateSelection();
        showGroup(id);
    });
    rebuildList();
    showEmpty();
}

int GroupsPageQS::badgeCount() const {
    const GroupCapabilityQS* g = context_.svc().groupCapability();
    return g ? g->pendingCount(context_.me()) : 0;
}

void GroupsPageQS::updateSelection() {
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

void GroupsPageQS::rebuildList() {
    list_->clear();
    ServiceBaseQS& s = context_.svc();
    GroupCapabilityQS* g = s.groupCapability();
    const std::string me = context_.me();
    const bool mine = tabs_->currentIndex() == 0;
    const QString keyword = search_->text().trimmed();
    int shown = 0;
    for (const GroupQS& group : mine ? g->groupsOf(me) : g->directory()) {
        if (!keyword.isEmpty() && !q(group.name() + " " + group.id()).contains(keyword, Qt::CaseInsensitive)) continue;
        ++shown;
        auto* item = new QListWidgetItem(list_);
        item->setSizeHint(QSize(0, ListRowQS::rowHeight()));
        item->setData(kKeyRole, q(group.id()));
        auto* row = new ListRowQS(list_);
        row->setAvatar(groupAvatar(s, group, 46));
        row->setTitle(q(group.name()));
        row->setTag(q(group.policy().displayName()).remove(QString::fromUtf8("模式")), group.policy().kind() != g->defaultPolicy());
        QString sub = QString::fromUtf8("群号 %1 · %2人").arg(q(group.id())).arg(group.members().size());
        GroupRoleQS role = GroupRoleQS::Member;
        if (group.effectiveRole(me, role) && role != GroupRoleQS::Member) sub += QString::fromUtf8(" · 我是") + q(roleName(role));
        if (!mine && group.hasMember(me)) sub += QString::fromUtf8(" · 已加入");
        if (group.hasInvitation(me)) sub += QString::fromUtf8(" · 邀请你加入");
        row->setSubtitle(sub);
        int pending = group.hasInvitation(me) ? 1 : 0;
        if (group.effectiveRole(me, role) && group.policy().canApprove(role) &&
            group.policy().joinMode() == JoinModeQS::Application) {
            pending += static_cast<int>(group.applications().size());
        }
        row->setBadge(pending);
        list_->setItemWidget(item, row);
    }
    if (shown == 0) {
        auto* item = new QListWidgetItem(mine ? QString::fromUtf8("  还没有加入任何群，去「群目录」看看")
                                              : QString::fromUtf8("  没有找到群"),
                                         list_);
        item->setFlags(Qt::NoItemFlags);
        item->setForeground(ThemeQS::subtext());
    }
    updateSelection();
}

void GroupsPageQS::clearDetail() {
    LayoutUtilQS::clear(detailLayout_);
}

void GroupsPageQS::showEmpty() {
    clearDetail();
    selected_.clear();
    detailLayout_->addStretch(1);
    auto* icon = new QLabel(detail_);
    icon->setPixmap(IconQS::pixmap("group", ThemeQS::soft(context_.accent).darker(115), 72));
    icon->setAlignment(Qt::AlignCenter);
    detailLayout_->addWidget(icon);
    auto* t = label(QString::fromUtf8("选择一个群查看详情"), "EmptyTitle", detail_);
    t->setAlignment(Qt::AlignCenter);
    detailLayout_->addWidget(t);
    const GroupCapabilityQS* g = context_.svc().groupCapability();
    auto policy = GroupPolicyRegistryQS::instance().create(g->defaultPolicy());
    auto* s = label(QString::fromUtf8("%1新建的群默认使用「%2」").arg(q(context_.svc().name()), q(policy->displayName())), "SubTitle", detail_);
    s->setAlignment(Qt::AlignCenter);
    detailLayout_->addWidget(s);
    detailLayout_->addStretch(1);
}

void GroupsPageQS::createGroup() {
    QMap<QString, QString> out;
    if (!FormDialogQS::ask(this, QString::fromUtf8("新建群"), QString::fromUtf8("你将成为群主，群号由系统分配"),
                           {{"name", QString::fromUtf8("群名称"), QString::fromUtf8("例如：课设答辩小组")}}, out,
                           QString::fromUtf8("创建")))
        return;
    std::string id;
    const ResultQS r = context_.svc().groupCapability()->createGroup(context_.me(), out["name"].toStdString(), id);
    context_.show(r);
    if (r) {
        tabs_->setCurrentIndex(0);
        selected_ = id;
        context_.refreshAll();
    }
}

void GroupsPageQS::showGroup(const std::string& groupId) {
    clearDetail();
    ServiceBaseQS& s = context_.svc();
    GroupCapabilityQS* cap = s.groupCapability();
    GroupQS g;
    if (!cap->findGroup(groupId, g)) {
        showEmpty();
        return;
    }
    const std::string me = context_.me();
    GroupRoleQS myRole = GroupRoleQS::Member;
    const bool member = g.effectiveRole(me, myRole);
    const GroupPolicyQS& policy = g.policy();

    // —— 头部 ——
    QVBoxLayout* inner = nullptr;
    QWidget* head = makeCard(detail_, inner);
    auto* top = new QHBoxLayout();
    top->setSpacing(16);
    auto* avatar = new QLabel(head);
    avatar->setPixmap(groupAvatar(s, g, 64));
    auto* names = new QVBoxLayout();
    auto* name = label(q(g.name()), "PanelTitle", head);
    name->setStyleSheet("font-size: 20px;");
    names->addWidget(name);
    names->addWidget(label(QString::fromUtf8("群号 %1 · %2人 · 创建于 %3%4")
                               .arg(q(g.id()))
                               .arg(g.members().size())
                               .arg(q(g.createdOn()))
                               .arg(member ? QString::fromUtf8(" · 我是") + q(roleName(myRole)) : QString()),
                           "SubTitle", head));
    top->addWidget(avatar);
    top->addLayout(names, 1);
    auto* actions = new QHBoxLayout();
    if (member && s.supportsChat()) {
        auto* chat = new QPushButton(IconQS::icon("chat", Qt::white, 18), QString::fromUtf8("进入群聊"), head);
        chat->setObjectName("PrimaryButton");
        connect(chat, &QPushButton::clicked, this, [this, groupId] { context_.openConversation("g:" + groupId); });
        actions->addWidget(chat);
    }
    if (!member) {
        QString joinText;
        switch (policy.joinMode()) {
            case JoinModeQS::Application: joinText = QString::fromUtf8("申请加入"); break;
            case JoinModeQS::Open: joinText = QString::fromUtf8("加入粉丝群"); break;
            case JoinModeQS::InviteOnly: joinText = QString::fromUtf8("仅限邀请加入"); break;
        }
        auto* join = new QPushButton(joinText, head);
        join->setObjectName("PrimaryButton");
        join->setEnabled(policy.joinMode() != JoinModeQS::InviteOnly && !g.hasApplication(me));
        if (g.hasApplication(me)) join->setText(QString::fromUtf8("已申请，等待审批"));
        connect(join, &QPushButton::clicked, this, [this, groupId, policy = policy.joinMode()] {
            QString note;
            if (policy == JoinModeQS::Application) {
                QMap<QString, QString> out;
                if (!FormDialogQS::ask(this, QString::fromUtf8("申请加入"), QString::fromUtf8("群主或管理员审批后入群"),
                                       {{"note", QString::fromUtf8("申请理由"), QString::fromUtf8("可留空")}}, out,
                                       QString::fromUtf8("提交申请")))
                    return;
                note = out["note"];
            }
            context_.show(context_.svc().groupCapability()->requestJoin(context_.me(), groupId, note.toStdString()));
            context_.refreshAll();
        });
        actions->addWidget(join);
    }
    top->addLayout(actions);
    inner->addLayout(top);
    if (!g.announcement().empty() || (member && policy.canEditAnnouncement(myRole))) {
        auto* ann = new QHBoxLayout();
        ann->addWidget(label(QString::fromUtf8("📢 ") + (g.announcement().empty() ? QString::fromUtf8("暂无群公告") : q(g.announcement())), "", head), 1);
        if (member && policy.canEditAnnouncement(myRole)) {
            auto* edit = new QPushButton(QString::fromUtf8("编辑公告"), head);
            edit->setObjectName("LinkButton");
            connect(edit, &QPushButton::clicked, this, [this, groupId, current = g.announcement()] {
                QMap<QString, QString> out;
                if (!FormDialogQS::ask(this, QString::fromUtf8("编辑群公告"), "", {{"text", QString::fromUtf8("公告内容"), "", q(current)}}, out))
                    return;
                context_.show(context_.svc().groupCapability()->setAnnouncement(context_.me(), groupId, out["text"].toStdString()));
                context_.refreshAll();
            });
            ann->addWidget(edit);
        }
        inner->addLayout(ann);
    }
    if (g.hasInvitation(me)) {
        auto* inv = new QHBoxLayout();
        inv->addWidget(label(QString::fromUtf8("有人邀请你加入这个群"), "RowTitle", head), 1);
        auto* no = new QPushButton(QString::fromUtf8("拒绝"), head);
        auto* yes = new QPushButton(QString::fromUtf8("接受邀请"), head);
        yes->setObjectName("PrimaryButton");
        inv->addWidget(no);
        inv->addWidget(yes);
        inner->addLayout(inv);
        connect(yes, &QPushButton::clicked, this, [this, groupId] {
            context_.show(context_.svc().groupCapability()->answerInvitation(context_.me(), groupId, true));
            context_.refreshAll();
        });
        connect(no, &QPushButton::clicked, this, [this, groupId] {
            context_.show(context_.svc().groupCapability()->answerInvitation(context_.me(), groupId, false));
            context_.refreshAll();
        });
    }
    detailLayout_->addWidget(head);

    // —— 群特色（由策略对象提供）+ 切换管理模式 ——
    QWidget* feature = makeCard(detail_, inner, "AccentCard");
    auto* featureHead = new QHBoxLayout();
    featureHead->addWidget(label(QString::fromUtf8("群特色 · ") + q(policy.displayName()), "RowTitle", feature), 1);
    if (member && myRole == GroupRoleQS::Owner) {
        auto* combo = new QComboBox(feature);
        for (const auto& kind : GroupPolicyRegistryQS::instance().kinds()) {
            auto p = GroupPolicyRegistryQS::instance().create(kind);
            combo->addItem(q(p->displayName()), q(kind));
            if (kind == policy.kind()) combo->setCurrentIndex(combo->count() - 1);
        }
        auto* apply = new QPushButton(QString::fromUtf8("切换管理模式"), feature);
        apply->setObjectName("PrimaryButton");
        featureHead->addWidget(combo);
        featureHead->addWidget(apply);
        connect(apply, &QPushButton::clicked, this, [this, groupId, combo] {
            const std::string kind = combo->currentData().toString().toStdString();
            if (!FormDialogQS::confirm(this, QString::fromUtf8("切换群管理模式"),
                                       QString::fromUtf8("切换为「%1」？群成员、管理员身份、讨论组和申请记录都会保留，只是按新模式的规则生效。")
                                           .arg(combo->currentText()),
                                       QString::fromUtf8("确认切换")))
                return;
            context_.show(context_.svc().groupCapability()->switchPolicy(context_.me(), groupId, kind));
            context_.refreshAll();
        });
    }
    inner->addLayout(featureHead);
    for (const auto& h : policy.highlights()) inner->addWidget(label(QString::fromUtf8("•  ") + q(h), "", feature));
    detailLayout_->addWidget(feature);

    // —— 待审批申请 ——
    if (member && policy.canApprove(myRole) && !g.applications().empty()) {
        QWidget* apps = makeCard(detail_, inner);
        inner->addWidget(label(QString::fromUtf8("入群申请（%1）").arg(g.applications().size()), "RowTitle", apps));
        for (const auto& a : g.applications()) {
            auto* line = new QHBoxLayout();
            auto* av = new QLabel(apps);
            av->setPixmap(AvatarQS::person(q(a.account), q(s.nicknameOf(a.account)), 34));
            line->addWidget(av);
            line->addWidget(label(q(s.nicknameOf(a.account) + "（" + a.account + "）  " + (a.note.empty() ? "" : "“" + a.note + "”")), "", apps), 1);
            auto* no = new QPushButton(QString::fromUtf8("拒绝"), apps);
            auto* yes = new QPushButton(QString::fromUtf8("同意"), apps);
            yes->setObjectName("PrimaryButton");
            line->addWidget(no);
            line->addWidget(yes);
            inner->addLayout(line);
            const std::string who = a.account;
            connect(yes, &QPushButton::clicked, this, [this, groupId, who] {
                context_.show(context_.svc().groupCapability()->reviewApplication(context_.me(), groupId, who, true));
                context_.refreshAll();
            });
            connect(no, &QPushButton::clicked, this, [this, groupId, who] {
                context_.show(context_.svc().groupCapability()->reviewApplication(context_.me(), groupId, who, false));
                context_.refreshAll();
            });
        }
        detailLayout_->addWidget(apps);
    } else if (member && !g.applications().empty() && policy.joinMode() != JoinModeQS::Application) {
        detailLayout_->addWidget(label(QString::fromUtf8("有 %1 条入群申请在当前模式下暂停处理，切回 QQ 群模式后可继续审批").arg(g.applications().size()),
                                       "SubTitle", detail_));
    }

    // —— 成员 ——
    QWidget* members = makeCard(detail_, inner);
    auto* memberHead = new QHBoxLayout();
    memberHead->addWidget(label(QString::fromUtf8("群成员（%1）").arg(g.members().size()), "RowTitle", members), 1);
    if (member && policy.canInvite(myRole)) {
        auto* invite = new QPushButton(IconQS::icon("user-add", ThemeQS::dark(context_.accent), 16), QString::fromUtf8("邀请好友"), members);
        invite->setObjectName("SoftButton");
        memberHead->addWidget(invite);
        connect(invite, &QPushButton::clicked, this, [this, groupId] {
            QStringList choices;
            std::vector<std::string> accounts;
            GroupQS fresh;
            context_.svc().groupCapability()->findGroup(groupId, fresh);
            if (const FriendCapabilityQS* f = context_.svc().friendCapability()) {
                for (const auto& v : f->friendsOf(context_.me())) {
                    if (fresh.hasMember(v.account) || fresh.hasInvitation(v.account)) continue;
                    choices << q(v.displayName() + "（" + v.account + "）");
                    accounts.push_back(v.account);
                }
            }
            if (choices.isEmpty()) {
                context_.toast(QString::fromUtf8("没有可以邀请的好友了"), false);
                return;
            }
            QMap<QString, QString> out;
            if (!FormDialogQS::ask(this, QString::fromUtf8("邀请好友入群"), QString::fromUtf8("对方确认后即可入群"),
                                   {{"who", QString::fromUtf8("选择好友"), "", "", false, choices}}, out, QString::fromUtf8("发送邀请")))
                return;
            const int index = static_cast<int>(choices.indexOf(out["who"]));
            if (index < 0) return;
            context_.show(context_.svc().groupCapability()->invite(context_.me(), groupId, accounts[static_cast<std::size_t>(index)]));
            context_.refreshAll();
        });
    }
    inner->addLayout(memberHead);
    auto* grid = new QGridLayout();
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(10);
    int index = 0;
    for (const auto& m : g.members()) {
        GroupRoleQS effective = m.role();
        g.effectiveRole(m.account(), effective);
        auto* chip = new QPushButton(members);
        chip->setObjectName("ChipButton");
        chip->setIcon(QIcon(AvatarQS::person(q(m.account()), q(s.nicknameOf(m.account())), 24)));
        QString text = q(context_.client->displayNameOf(context_.service, m.account()));
        if (effective != GroupRoleQS::Member) text += QString::fromUtf8(" · ") + q(roleName(effective));
        if (effective != m.role()) text += QString::fromUtf8("（保留）");
        if (m.account() == me) text += QString::fromUtf8("（我）");
        chip->setText(text);
        chip->setToolTip(q(s.accountLabel() + " " + m.account() + "，" + m.joinedOn() + " 入群"));
        grid->addWidget(chip, index / 3, index % 3);
        ++index;
        if (!member || m.account() == me) continue;
        // 成员操作菜单：按钮是否可用完全由策略对象判断
        const std::string target = m.account();
        const GroupRoleQS targetRole = effective;
        const GroupRoleQS storedRole = m.role();
        connect(chip, &QPushButton::clicked, this, [this, groupId, target, targetRole, storedRole, myRole, chip] {
            GroupQS fresh;
            if (!context_.svc().groupCapability()->findGroup(groupId, fresh)) return;
            const GroupPolicyQS& p = fresh.policy();
            QMenu menu(this);
            QAction* chat = nullptr;
            if (context_.svc().supportsChat() && context_.svc().friendCapability()->areFriends(context_.me(), target)) {
                chat = menu.addAction(QString::fromUtf8("发消息"));
            }
            QAction* admin = nullptr;
            if (p.canSetAdmin(myRole) && targetRole != GroupRoleQS::Owner) {
                admin = menu.addAction(storedRole == GroupRoleQS::Admin ? QString::fromUtf8("取消管理员") : QString::fromUtf8("设为管理员"));
            }
            QAction* owner = myRole == GroupRoleQS::Owner ? menu.addAction(QString::fromUtf8("转让群主给TA")) : nullptr;
            QAction* kick = p.canKick(myRole, targetRole) ? menu.addAction(QString::fromUtf8("移出本群")) : nullptr;
            if (menu.actions().isEmpty()) {
                menu.addAction(QString::fromUtf8("当前模式下你对该成员没有管理权限"))->setEnabled(false);
            }
            QAction* chosen = menu.exec(chip->mapToGlobal(QPoint(0, chip->height())));
            if (chosen == nullptr) return;
            GroupCapabilityQS* cap = context_.svc().groupCapability();
            if (chosen == chat) context_.openConversation("p:" + target);
            if (chosen == admin) context_.show(cap->setAdmin(context_.me(), groupId, target, storedRole != GroupRoleQS::Admin));
            if (chosen == owner &&
                FormDialogQS::confirm(this, QString::fromUtf8("转让群主"), QString::fromUtf8("转让后你将成为普通成员，确定吗？"))) {
                context_.show(cap->transferOwner(context_.me(), groupId, target));
            }
            if (chosen == kick &&
                FormDialogQS::confirm(this, QString::fromUtf8("移出成员"), QString::fromUtf8("确定将该成员移出本群？"), QString::fromUtf8("移出"), true)) {
                context_.show(cap->kick(context_.me(), groupId, target));
            }
            context_.refreshAll();
        });
    }
    inner->addLayout(grid);
    if (member) inner->addWidget(label(QString::fromUtf8("点击成员可进行管理操作（可用操作由当前群管理模式决定）"), "SubTitle", members));
    detailLayout_->addWidget(members);

    // —— 讨论组（子群）——
    if (!g.subgroups().empty() || (member && policy.supportsSubgroups())) {
        QWidget* subs = makeCard(detail_, inner);
        auto* subHead = new QHBoxLayout();
        subHead->addWidget(label(QString::fromUtf8("临时讨论组") + (policy.supportsSubgroups() ? QString() : QString::fromUtf8("（当前模式下休眠）")),
                                 "RowTitle", subs),
                           1);
        if (member && policy.supportsSubgroups()) {
            auto* create = new QPushButton(QString::fromUtf8("发起讨论组"), subs);
            create->setObjectName("SoftButton");
            subHead->addWidget(create);
            connect(create, &QPushButton::clicked, this, [this, groupId] {
                GroupQS fresh;
                context_.svc().groupCapability()->findGroup(groupId, fresh);
                QDialog dialog(this);
                dialog.setWindowTitle(QString::fromUtf8("发起讨论组"));
                auto* l = new QVBoxLayout(&dialog);
                l->setContentsMargins(24, 20, 24, 20);
                l->addWidget(label(QString::fromUtf8("发起讨论组"), "PanelTitle", &dialog));
                auto* nameEdit = new QLineEdit(&dialog);
                nameEdit->setPlaceholderText(QString::fromUtf8("讨论组名称"));
                l->addWidget(nameEdit);
                l->addWidget(label(QString::fromUtf8("选择成员（你会自动加入）"), "SubTitle", &dialog));
                std::vector<std::pair<QCheckBox*, std::string>> boxes;
                for (const auto& m : fresh.members()) {
                    if (m.account() == context_.me()) continue;
                    auto* box = new QCheckBox(q(context_.svc().nicknameOf(m.account()) + "（" + m.account() + "）"), &dialog);
                    l->addWidget(box);
                    boxes.emplace_back(box, m.account());
                }
                auto* row = new QHBoxLayout();
                row->addStretch(1);
                auto* cancel = new QPushButton(QString::fromUtf8("取消"), &dialog);
                auto* ok = new QPushButton(QString::fromUtf8("创建"), &dialog);
                ok->setObjectName("PrimaryButton");
                row->addWidget(cancel);
                row->addWidget(ok);
                l->addLayout(row);
                connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
                connect(ok, &QPushButton::clicked, &dialog, &QDialog::accept);
                if (dialog.exec() != QDialog::Accepted) return;
                std::vector<std::string> chosen;
                for (const auto& b : boxes) {
                    if (b.first->isChecked()) chosen.push_back(b.second);
                }
                context_.show(context_.svc().groupCapability()->createSubgroup(context_.me(), groupId, nameEdit->text().trimmed().toStdString(), chosen));
                context_.refreshAll();
            });
        }
        inner->addLayout(subHead);
        if (g.subgroups().empty()) inner->addWidget(label(QString::fromUtf8("还没有讨论组"), "SubTitle", subs));
        for (const auto& sub : g.subgroups()) {
            QStringList people;
            for (const auto& m : sub.members) people << q(s.nicknameOf(m));
            inner->addWidget(label(QString::fromUtf8("💬 %1　·　%2").arg(q(sub.name), people.join(QString::fromUtf8("、"))), "", subs));
        }
        detailLayout_->addWidget(subs);
    }

    // —— 退群 ——
    if (member) {
        auto* leave = new QPushButton(myRole == GroupRoleQS::Owner && g.members().size() == 1 ? QString::fromUtf8("解散群")
                                                                                              : QString::fromUtf8("退出该群"),
                                      detail_);
        leave->setObjectName("DangerButton");
        auto* row = new QHBoxLayout();
        row->addStretch(1);
        row->addWidget(leave);
        detailLayout_->addLayout(row);
        connect(leave, &QPushButton::clicked, this, [this, groupId] {
            if (!FormDialogQS::confirm(this, QString::fromUtf8("退出群"), QString::fromUtf8("确定退出该群？"), QString::fromUtf8("退出"), true)) return;
            const ResultQS r = context_.svc().groupCapability()->leave(context_.me(), groupId);
            context_.show(r);
            context_.refreshAll();
        });
    }
    detailLayout_->addStretch(1);
}

void GroupsPageQS::refresh() {
    rebuildList();
    if (!selected_.empty()) showGroup(selected_);
}

void GroupsPageQS::activate(const std::string& groupId) {
    search_->clear();
    GroupQS g;
    if (context_.svc().groupCapability()->findGroup(groupId, g)) tabs_->setCurrentIndex(g.hasMember(context_.me()) ? 0 : 1);
    selected_ = groupId;
    rebuildList();
    showGroup(groupId);
}

QS_REGISTER_GUI_FEATURE(GroupsPageQS, "groups")

}  // namespace qs
