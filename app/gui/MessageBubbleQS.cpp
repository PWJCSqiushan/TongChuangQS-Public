#include "MessageBubbleQS.h"

#include "DateQS.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

namespace qs {

QString MessageBubbleQS::statusLabel(const ChatMessageQS& m) {
    if (!m.outgoing) return {};
    if (m.isGroup()) {
        if (m.status == MessageStatusQS::Delivered) return QString::fromUtf8("✓✓ 全部送达");
        if (m.status == MessageStatusQS::Pending && m.ackedBy.empty()) return QString::fromUtf8("等待成员上线");
        return QString::fromUtf8("✓ 已送达 %1/%2").arg(m.ackedBy.size()).arg(m.recipients);
    }
    switch (m.status) {
        case MessageStatusQS::Pending: return QString::fromUtf8("◷ 对方上线后送达");
        case MessageStatusQS::Sent: return QString::fromUtf8("✓ 已发送");
        case MessageStatusQS::Delivered: return QString::fromUtf8("✓✓ 已送达");
        default: return {};
    }
}

MessageBubbleQS::MessageBubbleQS(const ChatMessageQS& m, const QPixmap& avatar, const QString& senderName,
                                 const QString& roleTag, QWidget* parent)
    : QWidget(parent), outgoing_(m.outgoing) {
    clock_ = QString::fromStdString(DateQS::clockText(m.time));
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(20, 4, 20, 4);
    row->setSpacing(10);

    auto* avatarLabel = new QLabel(this);
    avatarLabel->setPixmap(avatar);
    avatarLabel->setFixedSize(36, 36);
    avatarLabel->setAlignment(Qt::AlignTop);

    auto* column = new QVBoxLayout();
    column->setSpacing(3);
    if (m.isGroup() && !m.outgoing) {
        auto* nameRow = new QHBoxLayout();
        nameRow->setSpacing(6);
        auto* name = new QLabel(senderName, this);
        name->setObjectName("SenderName");
        nameRow->addWidget(name);
        if (!roleTag.isEmpty()) {
            auto* tag = new QLabel(roleTag, this);
            tag->setObjectName("Tag");
            nameRow->addWidget(tag);
        }
        nameRow->addStretch(1);
        column->addLayout(nameRow);
    }
    bubble_ = new QWidget(this);
    bubble_->setObjectName(m.outgoing ? "BubbleOut" : "BubbleIn");
    bubble_->setAttribute(Qt::WA_StyledBackground);
    auto* inner = new QVBoxLayout(bubble_);
    inner->setContentsMargins(12, 8, 12, 6);
    inner->setSpacing(2);
    text_ = new QLabel(QString::fromStdString(m.text), bubble_);
    text_->setObjectName("BubbleText");
    QFont bubbleFont = text_->font();
    bubbleFont.setPixelSize(14);
    text_->setFont(bubbleFont);
    text_->setWordWrap(true);
    text_->setTextFormat(Qt::PlainText);
    text_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    meta_ = new QLabel(bubble_);
    meta_->setObjectName("BubbleMeta");
    meta_->setAlignment(Qt::AlignRight);
    inner->addWidget(text_);
    inner->addWidget(meta_);
    column->addWidget(bubble_, 0, m.outgoing ? Qt::AlignRight : Qt::AlignLeft);

    if (m.outgoing) {
        row->addStretch(1);
        row->addLayout(column);
        row->addWidget(avatarLabel, 0, Qt::AlignTop);
    } else {
        row->addWidget(avatarLabel, 0, Qt::AlignTop);
        row->addLayout(column);
        row->addStretch(1);
    }
    updateStatus(m);
}

void MessageBubbleQS::updateStatus(const ChatMessageQS& m) {
    const QString status = statusLabel(m);
    meta_->setText(status.isEmpty() ? clock_ : clock_ + "  " + status);
}

void MessageBubbleQS::setMaxTextWidth(int width) {
    text_->setMaximumWidth(width);
    // 短消息让气泡收缩到文字宽度，长消息在最大宽度处换行
    const QFontMetrics fm(text_->font());  // 构造时已设为 14px，与样式表一致
    int natural = 0;
    for (const QString& line : text_->text().split('\n')) natural = qMax(natural, fm.horizontalAdvance(line));
    text_->setMinimumWidth(qMin(natural + 4, width));
}

void MessageBubbleQS::setHighlighted(bool on) {
    bubble_->setStyleSheet(on ? "border: 2px solid #F5A623;" : "");
}

}  // namespace qs
