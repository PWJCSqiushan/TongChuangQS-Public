#include "ChatsPageQS.h"

#include "AvatarQS.h"
#include "ChatViewQS.h"
#include "DateQS.h"
#include "IconQS.h"
#include "ListRowQS.h"
#include "ThemeQS.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QVBoxLayout>

namespace qs {

namespace {
const int kKeyRole = Qt::UserRole + 1;
const int kHighlightRole = Qt::UserRole + 2;

QWidget* emptyState(const QString& title, const QString& sub, const QColor& accent, QWidget* parent) {
    auto* w = new QWidget(parent);
    auto* l = new QVBoxLayout(w);
    l->addStretch(1);
    auto* icon = new QLabel(w);
    icon->setPixmap(IconQS::pixmap("chat", ThemeQS::soft(accent).darker(115), 72));
    icon->setAlignment(Qt::AlignCenter);
    auto* t = new QLabel(title, w);
    t->setObjectName("EmptyTitle");
    t->setAlignment(Qt::AlignCenter);
    auto* s = new QLabel(sub, w);
    s->setObjectName("SubTitle");
    s->setAlignment(Qt::AlignCenter);
    l->addWidget(icon);
    l->addSpacing(8);
    l->addWidget(t);
    l->addWidget(s);
    l->addStretch(1);
    return w;
}
}  // namespace

ChatsPageQS::ChatsPageQS(ShellContextQS& context, QWidget* parent) : FeaturePageQS(context, parent) {
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
    const auto features = context_.svc().features();
    QString title = QString::fromUtf8("消息");
    for (const auto& f : features) {
        if (f.kind == "chats") title = QString::fromStdString(f.title);
    }
    auto* heading = new QLabel(title, panel);
    heading->setObjectName("PanelTitle");
    plus_ = new QPushButton(panel);
    plus_->setObjectName("IconButton");
    plus_->setIcon(IconQS::icon("plus", ThemeQS::text(), 20));
    plus_->setToolTip(QString::fromUtf8("发起聊天"));
    plus_->setFixedSize(36, 36);
    head->addWidget(heading);
    head->addStretch(1);
    head->addWidget(plus_);
    p->addLayout(head);
    search_ = new QLineEdit(panel);
    search_->setObjectName("SearchBox");
    search_->setPlaceholderText(QString::fromUtf8("搜索会话与聊天记录"));
    search_->setClearButtonEnabled(true);
    search_->addAction(IconQS::icon("search", ThemeQS::subtext(), 16), QLineEdit::LeadingPosition);
    search_->setStyleSheet("padding-left: 6px;");
    p->addWidget(search_);
    list_ = new QListWidget(panel);
    list_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    p->addWidget(list_, 1);
    layout->addWidget(panel);

    right_ = new QStackedWidget(this);
    right_->setObjectName("ContentPanel");
    right_->addWidget(emptyState(QString::fromUtf8("选择一个会话开始聊天"),
                                 QString::fromUtf8("好友在线时消息即时送达，离线时会在对方上线后自动补发"),
                                 context_.accent, right_));
    view_ = new ChatViewQS(context_, right_);
    right_->addWidget(view_);
    layout->addWidget(right_, 1);

    connect(search_, &QLineEdit::textChanged, this, [this] { rebuildList(); });
    connect(plus_, &QPushButton::clicked, this, [this] { showNewChatMenu(); });
    connect(list_, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        const std::string key = item->data(kKeyRole).toString().toStdString();
        if (!key.empty()) openKey(key, item->data(kHighlightRole).toString().toStdString());
    });
    connect(list_, &QListWidget::currentRowChanged, this, [this] { updateSelection(); });
    rebuildList();
}

int ChatsPageQS::badgeCount() const {
    ChatHistoryQS* h = context_.client->history(context_.service);
    return h ? h->unreadTotal() : 0;
}

void ChatsPageQS::updateSelection() {
    for (int i = 0; i < list_->count(); ++i) {
        if (auto* row = dynamic_cast<ListRowQS*>(list_->itemWidget(list_->item(i)))) {
            row->setSelected(list_->item(i) == list_->currentItem());
        }
    }
}

void ChatsPageQS::rebuildList() {
    const std::string keyword = search_->text().trimmed().toStdString();
    list_->clear();
    ServiceBaseQS& s = context_.svc();
    const auto conversations = context_.client->conversations(context_.service);
    int selectedRow = -1;
    for (const auto& c : conversations) {
        if (!keyword.empty() && c.title.find(keyword) == std::string::npos && c.peer.find(keyword) == std::string::npos) continue;
        auto* item = new QListWidgetItem(list_);
        item->setSizeHint(QSize(0, ListRowQS::rowHeight()));
        item->setData(kKeyRole, QString::fromStdString(c.key));
        auto* row = new ListRowQS(list_);
        if (c.isGroup) {
            GroupQS g;
            QStringList seeds, names;
            if (s.groupCapability() && s.groupCapability()->findGroup(c.peer, g)) {
                for (const auto& m : g.members()) {
                    seeds << QString::fromStdString(m.account());
                    names << QString::fromStdString(s.nicknameOf(m.account()));
                }
            }
            row->setAvatar(AvatarQS::group(seeds, names, 46));
            row->setTag(QString::fromUtf8("群"));
        } else {
            row->setAvatar(AvatarQS::person(QString::fromStdString(c.peer), QString::fromStdString(s.nicknameOf(c.peer)), 46));
            row->setOnline(c.online);
        }
        row->setTitle(QString::fromStdString(c.title));
        row->setSubtitle(QString::fromStdString(c.preview));
        row->setTime(QString::fromStdString(DateQS::shortText(c.time)));
        row->setBadge(c.key == currentKey_ ? 0 : c.unread);
        list_->setItemWidget(item, row);
        if (c.key == currentKey_) selectedRow = list_->count() - 1;
    }
    // 搜索聊天记录（聊天记录查询）
    if (!keyword.empty()) {
        if (ChatHistoryQS* h = context_.client->history(context_.service)) {
            const auto hits = h->search(keyword);
            if (!hits.empty()) {
                auto* header = new QListWidgetItem(QString::fromUtf8("  聊天记录（%1 条）").arg(hits.size()), list_);
                header->setFlags(Qt::NoItemFlags);
                header->setForeground(ThemeQS::subtext());
            }
            for (std::size_t i = 0; i < hits.size() && i < 50; ++i) {
                const auto& hit = hits[i];
                auto* item = new QListWidgetItem(list_);
                item->setSizeHint(QSize(0, ListRowQS::rowHeight()));
                item->setData(kKeyRole, QString::fromStdString(hit.conversationKey));
                item->setData(kHighlightRole, QString::fromStdString(hit.message.id));
                auto* row = new ListRowQS(list_);
                row->setAvatar(AvatarQS::person(QString::fromStdString(hit.message.from),
                                                QString::fromStdString(hit.message.fromNick), 46));
                row->setTitle(QString::fromStdString(hit.conversationTitle));
                row->setSubtitle(QString::fromStdString((hit.message.outgoing ? "我：" : hit.message.fromNick + "：") + hit.message.text));
                row->setTime(QString::fromStdString(DateQS::shortText(hit.message.time)));
                list_->setItemWidget(item, row);
            }
        }
    }
    if (list_->count() == 0) {
        auto* item = new QListWidgetItem(keyword.empty() ? QString::fromUtf8("  还没有会话，点右上角 + 发起聊天")
                                                         : QString::fromUtf8("  没有找到相关内容"),
                                         list_);
        item->setFlags(Qt::NoItemFlags);
        item->setForeground(ThemeQS::subtext());
    }
    if (selectedRow >= 0) {
        list_->blockSignals(true);
        list_->setCurrentRow(selectedRow);
        list_->blockSignals(false);
    }
    updateSelection();
}

void ChatsPageQS::openKey(const std::string& key, const std::string& highlight) {
    currentKey_ = key;
    right_->setCurrentWidget(view_);
    view_->openConversation(key, highlight);
    rebuildList();
    context_.refreshAll();
}

void ChatsPageQS::activate(const std::string& conversationKey) {
    search_->clear();
    openKey(conversationKey);
}

void ChatsPageQS::showNewChatMenu() {
    ServiceBaseQS& s = context_.svc();
    QMenu menu(this);
    QMenu* friends = menu.addMenu(IconQS::icon("user", ThemeQS::text(), 18), QString::fromUtf8("和好友聊天"));
    if (const FriendCapabilityQS* f = s.friendCapability()) {
        auto list = f->friendsOf(context_.me());
        std::stable_sort(list.begin(), list.end(), [this](const FriendViewQS& a, const FriendViewQS& b) {
            return context_.client->isOnline(context_.service, a.account) && !context_.client->isOnline(context_.service, b.account);
        });
        for (const auto& v : list) {
            const bool online = context_.client->isOnline(context_.service, v.account);
            QAction* a = friends->addAction(QString::fromStdString(v.displayName() + "（" + v.account + "）") +
                                            (online ? QString::fromUtf8("  ● 在线") : QString()));
            const std::string key = "p:" + v.account;
            connect(a, &QAction::triggered, this, [this, key] { openKey(key); });
        }
        if (list.empty()) friends->addAction(QString::fromUtf8("暂无好友"))->setEnabled(false);
    }
    QMenu* groups = menu.addMenu(IconQS::icon("group", ThemeQS::text(), 18), QString::fromUtf8("进入群聊"));
    if (const GroupCapabilityQS* g = s.groupCapability()) {
        for (const auto& group : g->groupsOf(context_.me())) {
            QAction* a = groups->addAction(QString::fromStdString(group.name()) + QString::fromUtf8("（%1人）").arg(group.members().size()));
            const std::string key = "g:" + group.id();
            connect(a, &QAction::triggered, this, [this, key] { openKey(key); });
        }
    }
    menu.exec(plus_->mapToGlobal(QPoint(0, plus_->height() + 4)));
}

void ChatsPageQS::refresh() {
    rebuildList();
    view_->refreshHeader();
}

void ChatsPageQS::onChatEvents(const std::vector<ChatEventQS>& events) {
    bool listChanged = false;
    for (const ChatEventQS& e : events) {
        if (e.kind == ChatEventQS::KindQS::PresenceChanged) {
            listChanged = true;
            continue;
        }
        if (e.owner.service() != context_.service) continue;
        listChanged = true;
        if (e.conversation != view_->currentKey()) continue;
        if (e.kind == ChatEventQS::KindQS::MessageReceived) view_->appendIncoming(e.messageId);
        else view_->updateStatus(e.messageId);
    }
    if (listChanged) {
        rebuildList();
        view_->refreshHeader();
    }
}

QS_REGISTER_GUI_FEATURE(ChatsPageQS, "chats")

}  // namespace qs
