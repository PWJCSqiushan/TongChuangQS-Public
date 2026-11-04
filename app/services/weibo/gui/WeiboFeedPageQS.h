#pragma once

#include "FeaturePageQS.h"

#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>

namespace qs {

/** 微博"首页"：发微博 + 关注的人的动态时间线 + 点赞。属于微博服务自己的界面代码，自注册接入外壳。 */
class WeiboFeedPageQS : public FeaturePageQS {
public:
    WeiboFeedPageQS(ShellContextQS& context, QWidget* parent);
    void refresh() override;

private:
    void rebuildTimeline();
    QPlainTextEdit* editor_;
    QVBoxLayout* timeline_;
    QWidget* body_;
    QLabel* stats_;
};

}  // namespace qs
