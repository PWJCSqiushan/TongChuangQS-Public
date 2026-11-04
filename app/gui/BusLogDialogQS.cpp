#include "BusLogDialogQS.h"

#include "DateQS.h"
#include "PlatformQS.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

namespace qs {

BusLogDialogQS::BusLogDialogQS(PlatformQS& platform, QWidget* parent) : QDialog(parent), platform_(platform) {
    setWindowTitle(QString::fromUtf8("平台日志"));
    resize(720, 520);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 18);
    auto* title = new QLabel(QString::fromUtf8("服务总线通信记录"), this);
    title->setObjectName("PanelTitle");
    auto* sub = new QLabel(QString::fromUtf8("服务之间互不透明：注册联动、身份核验、好友推荐都以请求/应答消息经平台转发，下面是本窗口最近的记录。"), this);
    sub->setObjectName("SubTitle");
    sub->setWordWrap(true);
    list_ = new QListWidget(this);
    list_->setObjectName("PlainList");
    list_->setStyleSheet("QListWidget{background:#F7F8FA;border-radius:10px;padding:6px;font-family:Consolas,'Microsoft YaHei UI';font-size:12px;}");
    auto* buttons = new QHBoxLayout();
    auto* refresh = new QPushButton(QString::fromUtf8("刷新"), this);
    auto* close = new QPushButton(QString::fromUtf8("关闭"), this);
    close->setObjectName("PrimaryButton");
    buttons->addStretch(1);
    buttons->addWidget(refresh);
    buttons->addWidget(close);
    layout->addWidget(title);
    layout->addWidget(sub);
    layout->addWidget(list_, 1);
    layout->addLayout(buttons);
    connect(refresh, &QPushButton::clicked, this, [this] { reload(); });
    connect(close, &QPushButton::clicked, this, &QDialog::accept);
    reload();
}

void BusLogDialogQS::reload() {
    list_->clear();
    const auto& log = platform_.busLog();
    for (auto it = log.rbegin(); it != log.rend(); ++it) {
        const QString line = QString::fromUtf8("%1   %2 → %3   %4   %5%6")
                                 .arg(QString::fromStdString(DateQS::clockText(it->time)))
                                 .arg(QString::fromStdString(it->from), -8)
                                 .arg(QString::fromStdString(it->to), -8)
                                 .arg(QString::fromStdString(it->kind), -20)
                                 .arg(it->ok ? QString::fromUtf8("✓") : QString::fromUtf8("✗"))
                                 .arg(it->message.empty() ? QString() : QString::fromUtf8("  ") + QString::fromStdString(it->message));
        auto* item = new QListWidgetItem(line, list_);
        if (!it->ok) item->setForeground(QColor("#D14343"));
    }
    if (log.empty()) list_->addItem(QString::fromUtf8("暂无记录：登录、注册、绑定账号、推荐好友时会产生服务间请求"));
}

}  // namespace qs
