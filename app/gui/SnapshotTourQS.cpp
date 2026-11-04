#include "SnapshotTourQS.h"

#include "MainWindowQS.h"
#include "ServiceShellQS.h"
#include "UiAuditQS.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPushButton>
#include <QTabBar>

#include <QApplication>
#include <QDir>
#include <QThread>

namespace qs {

// 开发辅助：按固定路线打开各页面并截图，用于检查界面与制作答辩材料。
// 这是演示脚本而非平台代码，因此可以点名具体服务；平台本身不依赖这里。
bool SnapshotTourQS::run(MainWindowQS& w, const QString& directory) {
    if (!QDir().mkpath(directory)) return false;
    bool saved = true;
    const auto settle = [] {
        for (int i = 0; i < 12; ++i) {
            QApplication::processEvents();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            QThread::msleep(25);
        }
    };
    int index = 0;
    const auto save = [&](const QString& name) {
        settle();
        saved = w.grab().save(QString("%1/%2-%3.png").arg(directory).arg(++index, 2, 10, QChar('0')).arg(name)) && saved;
    };
    w.timer_.stop();
    w.showSelect();
    save("select");
    w.pickService("qq");
    save("qq-login");
    w.client_.login("qq", "10001", "123456");
    // 准备几条聊天记录（对方离线，显示为待送达）
    w.client_.sendToFriend("qq", "10002", QString::fromUtf8("晚上八点一起彩排答辩？").toStdString());
    w.client_.sendToFriend("qq", "10002", QString::fromUtf8("我把演示流程整理好了：先选服务，再登录，最后进入服务主界面。").toStdString());
    ChatMessageQS ignored;
    w.client_.sendToGroup("qq", "1001", QString::fromUtf8("大家好，答辩前我们在群里对一下流程").toStdString(), ignored);
    w.enterShell("qq", {});
    w.showSelect();
    save("select-after-login");
    ServiceShellQS* shell = w.shells_["qq"];
    w.enterShell("qq", {});
    shell->showFeature("chats", "p:10002");
    save("qq-chat");
    shell->showFeature("chats", "g:1001");
    save("qq-group-chat");
    shell->showFeature("contacts", "10002");
    save("qq-contact");
    shell->showFeature("groups", "1001");
    save("qq-group-detail");
    shell->showFeature("me");
    save("qq-me");
    w.client_.confirmPending("wechat");
    w.enterShell("wechat", {});
    w.shells_["wechat"]->showFeature("contacts");
    save("wechat-add-friend");
    w.shells_["wechat"]->showFeature("groups", "1002");
    save("wechat-group");
    if (w.client_.platform().service("weibo") == nullptr || w.client_.platform().service("weshop") == nullptr) {
        return saved;  // 微博/微商目录被移除时，平台照常运行，截图脚本到此结束
    }
    w.client_.confirmPending("weibo");
    w.enterShell("weibo", {});
    save("weibo-feed");
    w.client_.confirmPending("weshop");
    w.enterShell("weshop", {});
    save("weshop-store");
    w.pickService("wechat");
    w.switchUser();
    w.pickService("wechat");
    save("wechat-login");
    return saved;
}

bool SnapshotTourQS::audit(MainWindowQS& w, const QString& resultFile) {
    const auto settle = [] {
        for (int i = 0; i < 6; ++i) {
            QApplication::processEvents();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            QThread::msleep(15);
        }
    };
    QJsonArray checkpoints;
    int failures = 0;
    // 每个检查点：先刷新 3 次（模拟数据变化后的自动刷新），再找孤儿控件
    const auto check = [&](const QString& name, ServiceShellQS* shell) {
        settle();
        for (int i = 0; i < 3; ++i) {
            if (shell) shell->refreshAll();
            settle();
        }
        const QStringList orphans = UiAuditQS::findOrphans(&w);
        failures += orphans.isEmpty() ? 0 : 1;
        checkpoints.append(QJsonObject{{"name", name}, {"ok", orphans.isEmpty()}, {"orphans", QJsonArray::fromStringList(orphans)}});
    };
    const auto clickButton = [&](const QString& text) {
        for (QPushButton* b : w.findChildren<QPushButton*>()) {
            if (b->isVisible() && b->isEnabled() && b->text() == text) {
                b->click();
                return true;
            }
        }
        return false;
    };

    w.switchUser();
    w.showSelect();
    {
        // 检测器自检：故意放一个不在布局里的按钮，必须能被发现，否则巡检结果不可信
        QWidget* host = w.centralWidget();
        auto* planted = new QPushButton("planted-orphan", w.findChild<QWidget*>("SelectPage") ? w.findChild<QWidget*>("SelectPage") : host);
        planted->show();
        settle();
        bool found = false;
        for (const QString& o : UiAuditQS::findOrphans(&w)) found = found || o.contains("planted-orphan");
        planted->hide();
        delete planted;
        failures += found ? 0 : 1;
        checkpoints.append(QJsonObject{{"name", QString::fromUtf8("检测器自检（应能发现故意放置的孤儿按钮）")}, {"ok", found}});
    }
    {
        auto* host = new QWidget(w.centralWidget()); // deliberately has no layout
        host->resize(200, 80);
        auto* planted = new QPushButton("planted-without-layout", host);
        host->show(); planted->show(); settle();
        bool found = false;
        for (const auto& o : UiAuditQS::findOrphans(&w)) found = found || o.contains("planted-without-layout");
        delete host;
        failures += found ? 0 : 1;
        checkpoints.append(QJsonObject{{"name", "detector-without-layout"}, {"ok", found}});
    }
    check(QString::fromUtf8("服务选择页"), nullptr);
    for (const auto& s : w.client_.platform().services()) {
        w.pickService(s->id());
        check(QString::fromUtf8("登录页 · ") + QString::fromStdString(s->name()), nullptr);
        for (QTabBar* tabs : w.findChildren<QTabBar*>()) {
            if (tabs->isVisible() && tabs->count() == 2) {
                tabs->setCurrentIndex(1);
                settle();
                tabs->setCurrentIndex(0);
            }
        }
        check(QString::fromUtf8("登录/注册切换 · ") + QString::fromStdString(s->name()), nullptr);
    }

    // 登录 QQ，逐页巡检
    w.client_.login("qq", "10001", "123456");
    w.enterShell("qq", {});
    ServiceShellQS* qq = w.shells_["qq"];
    GroupCapabilityQS* groups = w.client_.platform().service("qq")->groupCapability();
    FriendCapabilityQS* friends = w.client_.platform().service("qq")->friendCapability();
    qq->showFeature("chats", "p:10002");
    w.client_.sendToFriend("qq", "10002", "audit");
    check("QQ · 私聊", qq);
    qq->showFeature("chats", "g:1001");
    check("QQ · 群聊", qq);
    qq->showFeature("contacts", "10002");
    check("QQ · 联系人详情", qq);
    friends->updateFriend("10001", "10002", QString::fromUtf8("林然（同桌）").toStdString(), QString::fromUtf8("同学").toStdString());
    check("QQ · 修改备注后", qq);
    qq->showFeature("contacts", "@requests");
    check("QQ · 新的朋友", qq);
    qq->showFeature("groups", "1001");
    check("QQ · 群详情 1001", qq);
    groups->reviewApplication("10001", "1001", "10008", true);
    check(QString::fromUtf8("QQ · 同意入群申请后"), qq);
    groups->switchPolicy("10001", "1001", "wechat");
    check(QString::fromUtf8("QQ · 切换为微信群模式"), qq);
    groups->switchPolicy("10001", "1001", "qq");
    check(QString::fromUtf8("QQ · 切回 QQ 群模式"), qq);
    groups->createSubgroup("10001", "1001", "audit", {"10002"});
    check(QString::fromUtf8("QQ · 新建讨论组后"), qq);
    qq->showFeature("groups", "1005");
    check(QString::fromUtf8("QQ · 群目录中未加入的群"), qq);
    qq->showFeature("me");
    check(QString::fromUtf8("QQ · 我"), qq);

    // 微信：推荐好友（按钮触发的局部刷新）
    w.client_.confirmPending("wechat");
    w.enterShell("wechat", {});
    ServiceShellQS* wechat = w.shells_["wechat"];
    wechat->showFeature("contacts");
    clickButton(QString::fromUtf8("查找"));
    settle();
    clickButton(QString::fromUtf8("查找"));
    check(QString::fromUtf8("微信 · 推荐好友（点两次查找）"), wechat);
    clickButton(QString::fromUtf8("共同好友"));
    check(QString::fromUtf8("微信 · 共同好友"), wechat);
    wechat->showFeature("groups", "1002");
    check(QString::fromUtf8("微信 · 群详情"), wechat);

    if (ServiceBaseQS* weibo = w.client_.platform().service("weibo")) {
        w.client_.confirmPending("weibo");
        w.enterShell("weibo", {});
        (void)weibo;
        check(QString::fromUtf8("微博 · 首页"), w.shells_["weibo"]);
        w.shells_["weibo"]->showFeature("groups", "1001");
        check(QString::fromUtf8("微博 · 粉丝群"), w.shells_["weibo"]);
    }
    if (w.client_.platform().service("weshop") != nullptr) {
        w.client_.confirmPending("weshop");
        w.enterShell("weshop", {});
        check(QString::fromUtf8("微商 · 小店"), w.shells_["weshop"]);
    }
    w.showSelect();
    check(QString::fromUtf8("服务选择页（多服务已登录）"), nullptr);

    QFile out(resultFile);
    if (!out.open(QIODevice::WriteOnly)) return false;
    const auto bytes = QJsonDocument(QJsonObject{{"ok", failures == 0}, {"failedCheckpoints", failures},
                                            {"checkpoints", checkpoints}}).toJson();
    return out.write(bytes) == bytes.size() && out.flush() && failures == 0;
}

}  // namespace qs
