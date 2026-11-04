#include "NavButtonQS.h"

#include "IconQS.h"

namespace qs {

NavButtonQS::NavButtonQS(const QString& icon, const QString& text, const QColor& accent, QWidget* parent)
    : QToolButton(parent) {
    setObjectName("NavButton");
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFixedSize(60, 58);
    setIcon(IconQS::toggleIcon(icon, QColor("#5F6670"), accent, 24));
    setIconSize(QSize(24, 24));
    setText(text);
    setToolTip(text);
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);  // 图标在上、文字在下
    badge_ = new QLabel(this);
    badge_->setObjectName("Badge");
    badge_->setAlignment(Qt::AlignCenter);
    badge_->setFixedHeight(18);
    badge_->setMinimumWidth(18);
    badge_->hide();
}

void NavButtonQS::setBadge(int count) {
    badge_->setText(count > 99 ? "99+" : QString::number(count));
    badge_->adjustSize();
    badge_->setVisible(count > 0);
    badge_->move(width() - badge_->width() - 6, 3);
}

void NavButtonQS::resizeEvent(QResizeEvent* event) {
    QToolButton::resizeEvent(event);
    badge_->move(width() - badge_->width() - 6, 3);
}

}  // namespace qs
