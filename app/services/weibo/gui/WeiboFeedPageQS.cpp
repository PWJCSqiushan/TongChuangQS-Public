#include "WeiboFeedPageQS.h"
#include "LayoutUtilQS.h"

#include "AvatarQS.h"
#include "DateQS.h"
#include "IconQS.h"
#include "ThemeQS.h"
#include "WeiboServiceQS.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>

namespace qs {

namespace {
QString q(const std::string& s) { return QString::fromStdString(s); }
}

WeiboFeedPageQS::WeiboFeedPageQS(ShellContextQS& context, QWidget* parent) : FeaturePageQS(context, parent) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setObjectName("ContentPanel");
    body_ = new QWidget(scroll);
    body_->setObjectName("ContentPanel");
    body_->setAttribute(Qt::WA_StyledBackground);
    auto* center = new QHBoxLayout(body_);
    center->setContentsMargins(0, 24, 0, 24);
    auto* column = new QVBoxLayout();
    column->setSpacing(14);
    center->addStretch(1);
    center->addLayout(column);
    center->addStretch(1);
    scroll->setWidget(body_);
    outer->addWidget(scroll);

    auto* composer = new QWidget(body_);
    composer->setObjectName("Card");
    composer->setAttribute(Qt::WA_StyledBackground);
    composer->setFixedWidth(640);
    auto* c = new QVBoxLayout(composer);
    c->setContentsMargins(20, 16, 20, 16);
    auto* head = new QHBoxLayout();
    auto* title = new QLabel(QString::fromUtf8("有什么新鲜事想分享？"), composer);
    title->setObjectName("RowTitle");
    stats_ = new QLabel(composer);
    stats_->setObjectName("SubTitle");
    head->addWidget(title);
    head->addStretch(1);
    head->addWidget(stats_);
    c->addLayout(head);
    editor_ = new QPlainTextEdit(composer);
    editor_->setPlaceholderText(QString::fromUtf8("最多140字"));
    editor_->setFixedHeight(80);
    c->addWidget(editor_);
    auto* row = new QHBoxLayout();
    row->addStretch(1);
    auto* post = new QPushButton(QString::fromUtf8("发布"), composer);
    post->setObjectName("PrimaryButton");
    row->addWidget(post);
    c->addLayout(row);
    column->addWidget(composer);
    timeline_ = new QVBoxLayout();
    timeline_->setSpacing(12);
    column->addLayout(timeline_);
    column->addStretch(1);
    connect(post, &QPushButton::clicked, this, [this] {
        auto& weibo = dynamic_cast<WeiboServiceQS&>(context_.svc());
        const ResultQS r = weibo.post(context_.me(), editor_->toPlainText().trimmed().toStdString());
        context_.show(r);
        if (r) {
            editor_->clear();
            rebuildTimeline();
        }
    });
    rebuildTimeline();
}

void WeiboFeedPageQS::refresh() { rebuildTimeline(); }

void WeiboFeedPageQS::rebuildTimeline() {
    LayoutUtilQS::clear(timeline_);
    auto& weibo = dynamic_cast<WeiboServiceQS&>(context_.svc());
    const std::string me = context_.me();
    stats_->setText(QString::fromUtf8("关注 %1 · 粉丝 %2")
                        .arg(weibo.follows().followingCount(me))
                        .arg(weibo.follows().followerCount(me)));
    const auto posts = weibo.timeline(me);
    if (posts.empty()) {
        auto* empty = new QLabel(QString::fromUtf8("关注的人还没有发微博，去「关注」页关注更多人吧"), body_);
        empty->setObjectName("SubTitle");
        timeline_->addWidget(empty);
    }
    for (const WeiboPostQS& p : posts) {
        auto* card = new QWidget(body_);
        card->setObjectName("Card");
        card->setAttribute(Qt::WA_StyledBackground);
        card->setFixedWidth(640);
        auto* l = new QVBoxLayout(card);
        l->setContentsMargins(20, 16, 20, 12);
        auto* head = new QHBoxLayout();
        auto* avatar = new QLabel(card);
        avatar->setPixmap(AvatarQS::person(q(p.author), q(weibo.nicknameOf(p.author)), 40));
        auto* names = new QVBoxLayout();
        names->setSpacing(0);
        auto* name = new QLabel(q(weibo.nicknameOf(p.author)), card);
        name->setObjectName("RowTitle");
        auto* when = new QLabel(q(DateQS::shortText(p.time)) + (p.author == me ? QString::fromUtf8(" · 我发布的") : QString()), card);
        when->setObjectName("SubTitle");
        names->addWidget(name);
        names->addWidget(when);
        head->addWidget(avatar);
        head->addLayout(names, 1);
        l->addLayout(head);
        auto* text = new QLabel(q(p.text), card);
        text->setWordWrap(true);
        text->setStyleSheet("font-size:14px;padding:6px 0;");
        text->setTextInteractionFlags(Qt::TextSelectableByMouse);
        l->addWidget(text);
        auto* actions = new QHBoxLayout();
        const bool liked = p.likes.count(me) != 0;
        auto* like = new QPushButton(IconQS::icon("heart", liked ? QColor("#E5484D") : ThemeQS::subtext(), 16),
                                     QString::number(p.likes.size()), card);
        like->setObjectName("IconButton");
        if (liked) like->setStyleSheet("color:#E5484D;");
        actions->addWidget(like);
        actions->addStretch(1);
        const std::string id = p.id;
        connect(like, &QPushButton::clicked, this, [this, id] {
            dynamic_cast<WeiboServiceQS&>(context_.svc()).toggleLike(context_.me(), id);
            rebuildTimeline();
        });
        if (p.author == me) {
            auto* del = new QPushButton(QString::fromUtf8("删除"), card);
            del->setObjectName("LinkButton");
            actions->addWidget(del);
            connect(del, &QPushButton::clicked, this, [this, id] {
                context_.show(dynamic_cast<WeiboServiceQS&>(context_.svc()).deletePost(context_.me(), id));
                rebuildTimeline();
            });
        }
        l->addLayout(actions);
        timeline_->addWidget(card);
    }
}

QS_REGISTER_GUI_FEATURE(WeiboFeedPageQS, "weibo.feed")

}  // namespace qs
