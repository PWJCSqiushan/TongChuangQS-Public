#include "ChatSelfTestQS.h"

#include "ChatHistoryQS.h"
#include "MainWindowQS.h"
#include "ServiceShellQS.h"

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProcess>
#include <QThread>

#include <functional>

namespace qs {

namespace {

/** 让事件循环运行（定时器会驱动网络收发），直到条件成立或超时。 */
bool waitUntil(const std::function<bool()>& done, int timeoutMs) {
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs) {
        QApplication::processEvents(QEventLoop::AllEvents, 50);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        if (done()) return true;
        QThread::msleep(20);
    }
    return false;
}

/** 当前窗口中可见的、文字等于 text 的消息气泡。 */
bool bubbleVisible(QWidget& root, const QString& text) {
    for (QLabel* label : root.findChildren<QLabel*>("BubbleText")) {
        if (label->isVisible() && label->text() == text) return true;
    }
    return false;
}

/** 找到当前可见的输入框，写入文字并模拟按下 Enter（走真实的按键处理路径）。 */
bool typeAndPressEnter(QWidget& root, const QString& text) {
    for (QPlainTextEdit* editor : root.findChildren<QPlainTextEdit*>("ComposerEdit")) {
        if (!editor->isVisible() || !editor->isEnabled()) continue;
        editor->setFocus();
        editor->setPlainText(text);
        QKeyEvent press(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
        QApplication::sendEvent(editor, &press);
        return editor->toPlainText().isEmpty();  // 发送成功后输入框被清空
    }
    return false;
}

const ChatMessageQS* lastMessage(ClientQS& client, const std::string& key, bool outgoing) {
    ChatHistoryQS* h = client.history("qq");
    ConversationQS* c = h ? h->conversation(key) : nullptr;
    if (c == nullptr) return nullptr;
    for (auto it = c->messages.rbegin(); it != c->messages.rend(); ++it) {
        if (it->outgoing == outgoing) return &*it;
    }
    return nullptr;
}

}  // namespace

int ChatSelfTestQS::runRole(MainWindowQS& w, const QString& role, const QString& resultFile) {
    const bool isA = role == "A";
    const std::string me = isA ? "10001" : "10002";
    const std::string peer = isA ? "10002" : "10001";
    const std::string key = "p:" + peer;
    const QString mine = isA ? QString::fromUtf8("你好，我是丘山（窗口A），今晚彩排？") : QString::fromUtf8("收到！我是林然（窗口B），八点见");
    const QString theirs = isA ? QString::fromUtf8("收到！我是林然（窗口B），八点见") : QString::fromUtf8("你好，我是丘山（窗口A），今晚彩排？");
    QJsonObject result{{"role", role}, {"account", QString::fromStdString(me)}};
    QJsonArray steps;
    const auto step = [&](const QString& name, bool ok) {
        steps.append(QJsonObject{{"step", name}, {"ok", ok}, {"time", QDateTime::currentDateTime().toString("HH:mm:ss.zzz")}});
        return ok;
    };

    bool ok = step(QString::fromUtf8("登录 QQ ") + QString::fromStdString(me), w.client_.login("qq", me, "123456").isOk());
    if (ok) {
        w.enterShell("qq", {});
        w.shells_["qq"]->showFeature("chats", key);
        ok = step(QString::fromUtf8("自动发现对方在线（无需填写地址端口）"),
                  waitUntil([&] { return w.client_.isOnline("qq", peer); }, 20000));
    }
    if (ok && isA) {
        ok = step(QString::fromUtf8("在输入框输入并按 Enter 发送"), typeAndPressEnter(w, mine));
        ok = ok && step(QString::fromUtf8("自己的气泡出现"), waitUntil([&] { return bubbleVisible(w, mine); }, 3000));
        ok = ok && step(QString::fromUtf8("收到送达回执（✓✓）"), waitUntil([&] {
                            const ChatMessageQS* m = lastMessage(w.client_, key, true);
                            return m != nullptr && m->status == MessageStatusQS::Delivered;
                        }, 15000));
        ok = ok && step(QString::fromUtf8("对方回复以气泡出现"), waitUntil([&] { return bubbleVisible(w, theirs); }, 20000));
    }
    if (ok && !isA) {
        ok = step(QString::fromUtf8("对方消息以气泡出现"), waitUntil([&] { return bubbleVisible(w, theirs); }, 25000));
        ok = ok && step(QString::fromUtf8("在输入框输入并按 Enter 回复"), typeAndPressEnter(w, mine));
        ok = ok && step(QString::fromUtf8("回复收到送达回执（✓✓）"), waitUntil([&] {
                            const ChatMessageQS* m = lastMessage(w.client_, key, true);
                            return m != nullptr && m->status == MessageStatusQS::Delivered;
                        }, 15000));
    }
    // 给对方留出时间完成最后一步再退出
    waitUntil([] { return false; }, 2500);
    result["steps"] = steps;
    result["ok"] = ok;
    result["port"] = static_cast<int>(w.client_.chat().port());
    QFile out(resultFile);
    if (!out.open(QIODevice::WriteOnly)) return 1;
    const auto bytes = QJsonDocument(result).toJson();
    const bool written = out.write(bytes) == bytes.size() && out.flush();
    return ok && written ? 0 : 1;
}

bool ChatSelfTestQS::orchestrate(const QString& exePath, const QString& workDir, const QString& summaryFile) {
    const QString run = QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss-zzz");
    const QString data = workDir + "/selftest-data-" + run;
    QDir().mkpath(data);
    QProcess a, b;
    const QString ra = workDir + "/selftest-A-" + run + ".json", rb = workDir + "/selftest-B-" + run + ".json";
    a.start(exePath, {"--data", data, "--window", "1", "--chat-selftest", "A", ra});
    QThread::msleep(800);
    b.start(exePath, {"--data", data, "--window", "2", "--chat-selftest", "B", rb});
    const bool finished = a.waitForFinished(90000) && b.waitForFinished(90000);
    const auto read = [](const QString& path) {
        QFile f(path);
        return f.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(f.readAll()).object() : QJsonObject{{"ok", false}, {"error", "no result"}};
    };
    const QJsonObject resultA = read(ra), resultB = read(rb);
    const bool ok = finished && a.exitStatus() == QProcess::NormalExit && b.exitStatus() == QProcess::NormalExit &&
                    a.exitCode() == 0 && b.exitCode() == 0 && resultA.value("ok").toBool() && resultB.value("ok").toBool();
    QJsonObject summary{{"ok", ok}, {"processesFinished", finished}, {"dataDirectory", data}, {"A", resultA}, {"B", resultB},
                        {"time", QDateTime::currentDateTime().toString(Qt::ISODate)}};
    QFile out(summaryFile);
    if (!out.open(QIODevice::WriteOnly)) return false;
    const auto bytes = QJsonDocument(summary).toJson();
    return out.write(bytes) == bytes.size() && out.flush() && ok;
}

}  // namespace qs
