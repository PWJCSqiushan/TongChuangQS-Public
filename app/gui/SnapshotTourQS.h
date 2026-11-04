#pragma once

#include <QString>

namespace qs {

class MainWindowQS;

/** 截图模式（TongChuangQS.exe --snapshots 目录）：依次打开各页面并保存 PNG。 */
class SnapshotTourQS {
public:
    static bool run(MainWindowQS& window, const QString& directory);
    /**
     * 界面巡检：逐页打开、反复刷新、执行会改变数据的操作，每一步都检查孤儿控件。
     * 结果写入 resultFile（JSON）。返回是否全部通过。
     */
    static bool audit(MainWindowQS& window, const QString& resultFile);
};

}  // namespace qs
