#pragma once

#include "FeaturePageQS.h"
#include "GroupQS.h"

#include <QLabel>
#include <QMap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace qs {

class MessageBubbleQS;

/**
 * 右侧聊天区：标题栏（头像、名称、在线状态/群信息）+ 消息气泡列表 + 输入框。
 * Enter 发送，Shift+Enter 换行；输入法选字时的回车由输入法处理，不会误发送。
 */
class ChatViewQS : public QWidget {
public:
    ChatViewQS(ShellContextQS& context, QWidget* parent);

    void openConversation(const std::string& key, const std::string& highlightMessageId = "");
    const std::string& currentKey() const { return key_; }
    void appendIncoming(const std::string& messageId);
    void updateStatus(const std::string& messageId);
    void refreshHeader();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void send();
    void rebuild(const std::string& highlight);
    void addBubble(const ChatMessageQS& message, std::int64_t& lastDay, bool animateScroll);
    void scrollToBottom();
    int maxBubbleWidth() const;
    QString roleTagFor(const std::string& account) const;

    ShellContextQS& context_;
    std::string key_;
    bool isGroup_ = false;
    std::string peer_;
    GroupQS group_;

    QLabel* avatar_;
    QLabel* title_;
    QLabel* subtitle_;
    QPushButton* infoButton_;
    QScrollArea* scroll_;
    QWidget* canvas_;
    QVBoxLayout* messages_;
    QPlainTextEdit* editor_;
    QPushButton* sendButton_;
    QMap<QString, MessageBubbleQS*> bubbles_;
    std::int64_t lastDay_ = 0;
};

}  // namespace qs
