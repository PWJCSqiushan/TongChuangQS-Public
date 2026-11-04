#include "ListRowQS.h"

#include <QHBoxLayout>
#include <QStyle>
#include <QVBoxLayout>

namespace qs {

ListRowQS::ListRowQS(QWidget* parent) : QWidget(parent) {
    setObjectName("ListRow");
    setAttribute(Qt::WA_TransparentForMouseEvents);
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 9, 12, 9);
    layout->setSpacing(12);

    auto* avatarBox = new QWidget(this);
    avatarBox->setObjectName("RowAvatarOverlay");
    avatarBox->setFixedSize(46, 46);
    avatar_ = new QLabel(avatarBox);
    avatar_->setObjectName("RowAvatarIcon");
    avatar_->setGeometry(0, 0, 46, 46);
    dot_ = new QLabel(avatarBox);
    dot_->setObjectName("RowPresenceDot");
    dot_->setGeometry(34, 34, 12, 12);
    dot_->setStyleSheet("background:#22C55E; border:2px solid white; border-radius:6px;");
    dot_->hide();
    layout->addWidget(avatarBox);

    auto* text = new QVBoxLayout();
    text->setSpacing(3);
    auto* top = new QHBoxLayout();
    top->setSpacing(6);
    title_ = new QLabel(this);
    title_->setObjectName("RowTitle");
    tag_ = new QLabel(this);
    tag_->setObjectName("Tag");
    tag_->hide();
    time_ = new QLabel(this);
    time_->setObjectName("RowTime");
    top->addWidget(title_);
    top->addWidget(tag_);
    top->addStretch(1);
    top->addWidget(time_);
    auto* bottom = new QHBoxLayout();
    bottom->setSpacing(6);
    subtitle_ = new QLabel(this);
    subtitle_->setObjectName("RowSubtitle");
    subtitle_->setMinimumWidth(10);
    badge_ = new QLabel(this);
    badge_->setObjectName("Badge");
    badge_->setAlignment(Qt::AlignCenter);
    badge_->setMinimumWidth(18);
    badge_->setFixedHeight(18);
    badge_->hide();
    bottom->addWidget(subtitle_, 1);
    bottom->addWidget(badge_);
    text->addLayout(top);
    text->addLayout(bottom);
    layout->addLayout(text, 1);
    setProperty("selected", false);
}

void ListRowQS::setAvatar(const QPixmap& avatar) { avatar_->setPixmap(avatar); }

void ListRowQS::setTitle(const QString& title) {
    QFont font = title_->font();
    font.setPixelSize(14);
    font.setWeight(QFont::Medium);
    const QFontMetrics fm(font);
    title_->setText(fm.elidedText(title, Qt::ElideRight, 150));
    title_->setToolTip(title);
}

void ListRowQS::setSubtitle(const QString& subtitle) {
    // 过长时省略，避免撑开列表
    const QFontMetrics fm(subtitle_->font());
    subtitle_->setText(fm.elidedText(subtitle, Qt::ElideRight, 210));
}

void ListRowQS::setTag(const QString& tag, bool gray) {
    tag_->setObjectName(gray ? "GrayTag" : "Tag");
    tag_->setText(tag);
    tag_->setVisible(!tag.isEmpty());
}

void ListRowQS::setTime(const QString& time) { time_->setText(time); }

void ListRowQS::setBadge(int count) {
    badge_->setText(count > 99 ? "99+" : QString::number(count));
    badge_->setVisible(count > 0);
}

void ListRowQS::setOnline(bool online, bool visible) { dot_->setVisible(visible && online); }

void ListRowQS::setSelected(bool selected) {
    setProperty("selected", selected);
    const auto children = findChildren<QWidget*>();
    for (QWidget* w : children) {
        w->style()->unpolish(w);
        w->style()->polish(w);
    }
    style()->unpolish(this);
    style()->polish(this);
}

}  // namespace qs
