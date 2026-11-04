#include "GuiApplicationQS.h"

#include "ClientQS.h"
#include "MainWindowQS.h"
#include "PathQS.h"
#include "SnapshotTourQS.h"
#include "ChatSelfTestQS.h"

#include <QFileInfo>
#include "ThemeQS.h"

#include <QApplication>
#include <QIcon>
#include <QMessageBox>
#include <QScreen>

namespace qs {

int GuiApplicationQS::run(int argc, char** argv) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("TongChuangQS");
    ThemeQS::applyBase(app);
    const QString exeDir = QString::fromStdString(PathQS::toUtf8(PathQS::executableDirectory()));
    QApplication::setWindowIcon(QIcon(exeDir + "/TongChuangQS.png"));

    // 参数：--data 数据目录   --window 编号（双窗口演示）   --lan 允许局域网连接   --snapshots 截图目录
    QString dataRoot = exeDir + "/data";
    QString windowTag;
    QString snapshots;
    QString selftestRole, selftestResult;
    bool lan = false;
    const QStringList args = QApplication::arguments();
    for (int i = 1; i < args.size(); ++i) {
        if (args[i] == "--data" && i + 1 < args.size()) dataRoot = args[++i];
        else if (args[i] == "--window" && i + 1 < args.size()) windowTag = QString::fromUtf8("窗口") + args[++i];
        else if (args[i] == "--snapshots" && i + 1 < args.size()) snapshots = args[++i];
        else if (args[i] == "--lan") lan = true;
        else if (args[i] == "--chat-selftest" && i + 2 < args.size()) {
            selftestRole = args[++i];
            selftestResult = args[++i];
        }
    }

    ClientQS client(dataRoot.toStdString());
    const ResultQS started = client.start(lan);
    if (!started) {
        if (snapshots.isEmpty() && selftestRole.isEmpty())
            QMessageBox::critical(nullptr, QString::fromUtf8("启动失败"), QString::fromStdString(started.message()));
        return 1;
    }
    MainWindowQS window(client, windowTag);
    window.resize(1200, 780);
    window.setMinimumSize(980, 640);
    if (!windowTag.isEmpty()) {
        // 双窗口演示：左右并排摆放
        const QRect screen = QApplication::primaryScreen()->availableGeometry();
        const int w = qMin(1100, screen.width() / 2);
        window.resize(w, qMin(760, screen.height() - 40));
        window.setMinimumSize(900, 600);
        window.move(screen.left() + (windowTag.endsWith('2') ? screen.width() - w : 0), screen.top() + 10);
    }
    if (!selftestRole.isEmpty()) {
        window.show();
        return ChatSelfTestQS::runRole(window, selftestRole, selftestResult);
    }
    if (!snapshots.isEmpty()) {
        window.resize(1280, 800);
        window.show();
        if (!SnapshotTourQS::run(window, snapshots)) return 1;
        // 界面巡检：逐页反复刷新并检查孤儿控件
        const bool uiOk = SnapshotTourQS::audit(window, QFileInfo(snapshots).absolutePath() + "/ui-audit.json");
        window.hide();
        // 截图之后再做一次双进程聊天自检，结果写到截图目录旁的 chat-selftest.json
        const QString work = QFileInfo(snapshots).absolutePath();
        const bool chatOk = ChatSelfTestQS::orchestrate(QApplication::applicationFilePath(), work, work + "/chat-selftest.json");
        return uiOk && chatOk ? 0 : 1;
    }
    window.show();
    if (!started) {
        QMessageBox::warning(&window, QString::fromUtf8("聊天功能不可用"), QString::fromStdString(started.message()));
    }
    return app.exec();
}

}  // namespace qs
