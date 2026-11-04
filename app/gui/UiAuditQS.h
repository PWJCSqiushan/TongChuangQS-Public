#pragma once

#include <QString>
#include <QStringList>

class QWidget;

namespace qs {

/**
 * 界面自检（开发辅助）：找出"孤儿控件"——它的父控件有布局，但它自己并不在该布局里。
 * 这类控件通常是刷新页面时没清理干净的旧控件，会停留在旧位置、和新内容重叠。
 */
class UiAuditQS {
public:
    /** 返回 root 下所有可见孤儿控件的描述（类名、对象名、文字、位置）。 */
    static QStringList findOrphans(QWidget* root);
};

}  // namespace qs
