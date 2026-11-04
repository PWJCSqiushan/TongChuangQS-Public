#pragma once

#include "ChatMessageQS.h"

#include <QLabel>
#include <QWidget>

namespace qs {

/**
 * 一条消息的气泡：收到的在左（白色）、发出的在右（主题浅色），
 * 群消息显示发送者昵称与群角色标签；发出的消息显示送达状态。
 */
class MessageBubbleQS : public QWidget {
public:
    MessageBubbleQS(const ChatMessageQS& message, const QPixmap& avatar, const QString& senderName,
                    const QString& roleTag, QWidget* parent);
    void updateStatus(const ChatMessageQS& message);
    void setMaxTextWidth(int width);
    void setHighlighted(bool on);

private:
    static QString statusLabel(const ChatMessageQS& message);
    QLabel* text_ = nullptr;
    QLabel* meta_ = nullptr;
    QWidget* bubble_ = nullptr;
    bool outgoing_ = false;
    QString clock_;
};

}  // namespace qs
