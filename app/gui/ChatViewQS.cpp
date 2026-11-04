#include "ChatViewQS.h"

#include "AvatarQS.h"
#include "DateQS.h"
#include "IconQS.h"
#include "LayoutUtilQS.h"
#include "MessageBubbleQS.h"
#include "ThemeQS.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QScrollBar>
#include <QTimer>

namespace qs {

namespace {
std::int64_t dayOf(std::int64_t millis) {
    const std::string d = DateQS::dateText(millis);
    return std::stoll(d.substr(0, 4) + d.substr(5, 2) + d.substr(8, 2));
}
QString dayLabel(std::int64_t millis) {
    const std::string d = DateQS::dateText(millis);
    if (d == DateQS::today()) return QString::fromUtf8("今天");
    if (d == DateQS::dateText(DateQS::nowMillis() - 86400000LL)) return QString::fromUtf8("昨天");
    return QString::fromStdString(d);
}
}  // namespace

ChatViewQS::ChatViewQS(ShellContextQS& context, QWidget* parent) : QWidget(parent), context_(context) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* header = new QWidget(this);
    header->setObjectName("ChatHeader");
    header->setAttribute(Qt::WA_StyledBackground);
    header->setFixedHeight(68);
    auto* h = new QHBoxLayout(header);
    h->setContentsMargins(20, 10, 16, 10);
    h->setSpacing(12);
    avatar_ = new QLabel(header);
    avatar_->setFixedSize(42, 42);
    auto* titles = new QVBoxLayout();
    titles->setSpacing(1);
    title_ = new QLabel(header);
    title_->setObjectName("PanelTitle");
    subtitle_ = new QLabel(header);
    subtitle_->setObjectName("SubTitle");
    titles->addWidget(title_);
    titles->addWidget(subtitle_);
    infoButton_ = new QPushButton(QString::fromUtf8("资料"), header);
    infoButton_->setIcon(IconQS::icon("info", ThemeQS::text(), 18));
    h->addWidget(avatar_);
    h->addLayout(titles, 1);
    h->addWidget(infoButton_);
    layout->addWidget(header);

    scroll_ = new QScrollArea(this);
    scroll_->setObjectName("ChatScroll");
    scroll_->setWidgetResizable(true);
    canvas_ = new QWidget(scroll_);
    canvas_->setObjectName("ChatCanvas");
    messages_ = new QVBoxLayout(canvas_);
    messages_->setContentsMargins(0, 16, 0, 16);
    messages_->setSpacing(4);
    messages_->addStretch(1);
    scroll_->setWidget(canvas_);
    layout->addWidget(scroll_, 1);

    auto* composer = new QWidget(this);
    composer->setObjectName("Composer");
    composer->setAttribute(Qt::WA_StyledBackground);
    auto* c = new QVBoxLayout(composer);
    c->setContentsMargins(20, 12, 20, 10);
    c->setSpacing(4);
    auto* row = new QHBoxLayout();
    row->setSpacing(10);
    editor_ = new QPlainTextEdit(composer);
    editor_->setObjectName("ComposerEdit");
    editor_->setPlaceholderText(QString::fromUtf8("输入消息，按 Enter 发送"));
    editor_->setFixedHeight(58);
    editor_->installEventFilter(this);
    sendButton_ = new QPushButton(QString::fromUtf8("发送"), composer);
    sendButton_->setObjectName("PrimaryButton");
    sendButton_->setIcon(IconQS::icon("send", Qt::white, 18));
    sendButton_->setFixedHeight(42);
    sendButton_->setMinimumWidth(92);
    row->addWidget(editor_, 1);
    row->addWidget(sendButton_, 0, Qt::AlignBottom);
    auto* hint = new QLabel(QString::fromUtf8("Enter 发送 · Shift + Enter 换行"), composer);
    hint->setObjectName("Hint");
    c->addLayout(row);
    c->addWidget(hint);
    layout->addWidget(composer);

    connect(sendButton_, &QPushButton::clicked, this, [this] { send(); });
    connect(infoButton_, &QPushButton::clicked, this, [this] {
        if (isGroup_) context_.navigate("groups", peer_);
        else context_.navigate("contacts", peer_);
    });
}

bool ChatViewQS::eventFilter(QObject* watched, QEvent* event) {
    if (watched == editor_ && event->type() == QEvent::KeyPress) {
        auto* key = static_cast<QKeyEvent*>(event);
        if ((key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) && !(key->modifiers() & Qt::ShiftModifier)) {
            send();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

int ChatViewQS::maxBubbleWidth() const { return qMax(220, static_cast<int>(scroll_->viewport()->width() * 0.58)); }

void ChatViewQS::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    const int w = maxBubbleWidth();
    for (MessageBubbleQS* b : bubbles_) b->setMaxTextWidth(w);
}

QString ChatViewQS::roleTagFor(const std::string& account) const {
    GroupRoleQS role = GroupRoleQS::Member;
    if (!isGroup_ || !group_.effectiveRole(account, role) || role == GroupRoleQS::Member) return {};
    return QString::fromStdString(roleName(role));
}

void ChatViewQS::refreshHeader() {
    if (key_.empty()) return;
    ServiceBaseQS& s = context_.svc();
    if (isGroup_) {
        if (s.groupCapability()) s.groupCapability()->findGroup(peer_, group_);
        QStringList seeds, names;
        for (const auto& m : group_.members()) {
            seeds << QString::fromStdString(m.account());
            names << QString::fromStdString(s.nicknameOf(m.account()));
        }
        avatar_->setPixmap(AvatarQS::group(seeds, names, 42));
        title_->setText(QString::fromStdString(group_.name()));
        subtitle_->setText(QString::fromUtf8("%1群 · %2人 · %3")
                               .arg(QString::fromStdString(s.name()))
                               .arg(group_.members().size())
                               .arg(QString::fromStdString(group_.policy().displayName())));
        const bool member = group_.hasMember(context_.me());
        editor_->setEnabled(member);
        sendButton_->setEnabled(member);
        if (!member) editor_->setPlaceholderText(QString::fromUtf8("你已不在该群中"));
    } else {
        const QString name = QString::fromStdString(context_.client->displayNameOf(context_.service, peer_));
        avatar_->setPixmap(AvatarQS::person(QString::fromStdString(peer_), QString::fromStdString(s.nicknameOf(peer_)), 42));
        title_->setText(name);
        const bool online = context_.client->isOnline(context_.service, peer_);
        subtitle_->setText(online ? QString::fromUtf8("● 在线") : QString::fromUtf8("○ 离线 · 消息将在对方上线后自动送达"));
        subtitle_->setStyleSheet(online ? "color:#22A35A;" : "");
        const bool friends = s.friendCapability() && s.friendCapability()->areFriends(context_.me(), peer_);
        editor_->setEnabled(friends);
        sendButton_->setEnabled(friends);
        if (!friends) editor_->setPlaceholderText(QString::fromUtf8("对方不是你的好友，无法发送消息"));
    }
    if (editor_->isEnabled()) editor_->setPlaceholderText(QString::fromUtf8("输入消息，按 Enter 发送"));
}

void ChatViewQS::openConversation(const std::string& key, const std::string& highlightMessageId) {
    key_ = key;
    isGroup_ = key.rfind("g:", 0) == 0;
    peer_ = key.substr(2);
    refreshHeader();
    rebuild(highlightMessageId);
    if (ChatHistoryQS* h = context_.client->history(context_.service)) h->markRead(key_);
    editor_->setFocus();
}

void ChatViewQS::rebuild(const std::string& highlight) {
    for (MessageBubbleQS* b : bubbles_) {
        b->hide();
        b->deleteLater();
    }
    bubbles_.clear();
    LayoutUtilQS::clear(messages_, 1);  // 保留第 0 项（顶部的伸缩空白）
    lastDay_ = 0;
    ChatHistoryQS* h = context_.client->history(context_.service);
    ConversationQS* conv = h ? h->conversation(key_) : nullptr;
    if (conv == nullptr || conv->messages.empty()) {
        auto* note = new QLabel(isGroup_ ? QString::fromUtf8("群里还没有消息，打个招呼吧")
                                         : QString::fromUtf8("你们已经是好友了，开始聊天吧"),
                                canvas_);
        note->setObjectName("SystemNote");
        note->setAlignment(Qt::AlignCenter);
        messages_->addWidget(note);
        return;
    }
    const std::size_t start = conv->messages.size() > 300 ? conv->messages.size() - 300 : 0;
    for (std::size_t i = start; i < conv->messages.size(); ++i) addBubble(conv->messages[i], lastDay_, false);
    MessageBubbleQS* target = highlight.empty() ? nullptr : bubbles_.value(QString::fromStdString(highlight));
    if (target != nullptr) {
        target->setHighlighted(true);
        QTimer::singleShot(60, this, [this, target] { scroll_->ensureWidgetVisible(target, 0, 120); });
    } else {
        scrollToBottom();
    }
}

void ChatViewQS::addBubble(const ChatMessageQS& m, std::int64_t& lastDay, bool animateScroll) {
    // 清除"还没有消息"的提示
    if (bubbles_.isEmpty() && messages_->count() > 1) {
        QLayoutItem* item = messages_->takeAt(1);
        if (item->widget()) { item->widget()->hide(); item->widget()->deleteLater(); }
        delete item;
    }
    const std::int64_t day = dayOf(m.time);
    if (day != lastDay) {
        lastDay = day;
        auto* holder = new QWidget(canvas_);
        auto* hl = new QHBoxLayout(holder);
        hl->setContentsMargins(0, 8, 0, 8);
        auto* chip = new QLabel(dayLabel(m.time), holder);
        chip->setObjectName("DateChip");
        hl->addStretch(1);
        hl->addWidget(chip);
        hl->addStretch(1);
        messages_->addWidget(holder);
    }
    ServiceBaseQS& s = context_.svc();
    const std::string sender = m.from;
    const QPixmap avatar = AvatarQS::person(QString::fromStdString(sender), QString::fromStdString(s.nicknameOf(sender)), 36);
    const QString name = QString::fromStdString(m.isGroup() ? context_.client->displayNameOf(context_.service, sender) : "");
    auto* bubble = new MessageBubbleQS(m, avatar, name.isEmpty() ? QString::fromStdString(m.fromNick) : name,
                                       roleTagFor(sender), canvas_);
    bubble->setMaxTextWidth(maxBubbleWidth());
    messages_->addWidget(bubble);
    bubbles_.insert(QString::fromStdString(m.id), bubble);
    if (animateScroll) scrollToBottom();
}

void ChatViewQS::scrollToBottom() {
    QTimer::singleShot(30, this, [this] { scroll_->verticalScrollBar()->setValue(scroll_->verticalScrollBar()->maximum()); });
}

void ChatViewQS::appendIncoming(const std::string& messageId) {
    ChatHistoryQS* h = context_.client->history(context_.service);
    ConversationQS* conv = h ? h->conversation(key_) : nullptr;
    if (conv == nullptr) return;
    for (auto it = conv->messages.rbegin(); it != conv->messages.rend(); ++it) {
        if (it->id == messageId) {
            if (!bubbles_.contains(QString::fromStdString(messageId))) addBubble(*it, lastDay_, true);
            break;
        }
    }
    h->markRead(key_);
}

void ChatViewQS::updateStatus(const std::string& messageId) {
    MessageBubbleQS* bubble = bubbles_.value(QString::fromStdString(messageId));
    ChatHistoryQS* h = context_.client->history(context_.service);
    if (bubble == nullptr || h == nullptr) return;
    if (const ChatMessageQS* m = h->find(messageId)) bubble->updateStatus(*m);
}

void ChatViewQS::send() {
    const QString text = editor_->toPlainText().trimmed();
    if (text.isEmpty() || key_.empty() || !editor_->isEnabled()) return;
    ChatMessageQS sent;
    if (isGroup_) {
        const ResultQS r = context_.client->sendToGroup(context_.service, peer_, text.toStdString(), sent);
        if (!r) {
            context_.show(r);
            return;
        }
    } else {
        sent = context_.client->sendToFriend(context_.service, peer_, text.toStdString());
    }
    if (sent.id.empty()) {
        context_.toast(QString::fromUtf8("发送失败：聊天服务未就绪或聊天记录无法保存"), false);
        return;
    }
    editor_->clear();
    addBubble(sent, lastDay_, true);
    context_.refreshAll();
}

}  // namespace qs
