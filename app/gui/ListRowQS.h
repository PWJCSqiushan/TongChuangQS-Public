#pragma once

#include <QLabel>
#include <QPixmap>
#include <QWidget>

namespace qs {

/**
 * 列表中的一行（会话、好友、群通用）：头像 | 标题 + 标签 + 时间 / 副标题 + 角标。
 * 选中时通过动态属性 selected 切换为主题色背景上的白色文字。
 */
class ListRowQS : public QWidget {
public:
    explicit ListRowQS(QWidget* parent = nullptr);

    void setAvatar(const QPixmap& avatar);
    void setTitle(const QString& title);
    void setSubtitle(const QString& subtitle);
    void setTag(const QString& tag, bool gray = false);
    void setTime(const QString& time);
    void setBadge(int count);
    void setOnline(bool online, bool visible = true);
    void setSelected(bool selected);
    static int rowHeight() { return 66; }

private:
    QLabel* avatar_;
    QLabel* dot_;
    QLabel* title_;
    QLabel* tag_;
    QLabel* time_;
    QLabel* subtitle_;
    QLabel* badge_;
};

}  // namespace qs
