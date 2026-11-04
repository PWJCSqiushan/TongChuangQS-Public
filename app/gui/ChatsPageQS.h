#pragma once

#include "FeaturePageQS.h"

#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>

namespace qs {

class ChatViewQS;

/**
 * "消息"页面（通用能力页，适用于所有 supportsChat() 的服务）：
 * 左侧会话列表（头像、未读角标、在线点、最后一条消息），支持搜索会话与聊天记录；
 * 右侧为聊天区 ChatViewQS。
 */
class ChatsPageQS : public FeaturePageQS {
public:
    ChatsPageQS(ShellContextQS& context, QWidget* parent);

    void refresh() override;
    void onChatEvents(const std::vector<ChatEventQS>& events) override;
    int badgeCount() const override;
    void activate(const std::string& conversationKey) override;
    void refreshPresence() override { refresh(); }

private:
    void rebuildList();
    void openKey(const std::string& key, const std::string& highlight = "");
    void showNewChatMenu();
    void updateSelection();

    QLineEdit* search_;
    QListWidget* list_;
    QPushButton* plus_;
    QStackedWidget* right_;
    ChatViewQS* view_;
    std::string currentKey_;
};

}  // namespace qs
