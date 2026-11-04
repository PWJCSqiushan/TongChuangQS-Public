#pragma once

#include <QLabel>
#include <QToolButton>

namespace qs {

/** 左侧导航栏按钮：图标在上、文字在下，右上角可显示红色角标。 */
class NavButtonQS : public QToolButton {
public:
    NavButtonQS(const QString& icon, const QString& text, const QColor& accent, QWidget* parent);
    void setBadge(int count);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    QLabel* badge_;
};

}  // namespace qs
