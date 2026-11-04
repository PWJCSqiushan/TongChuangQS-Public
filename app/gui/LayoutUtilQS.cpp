#include "LayoutUtilQS.h"

#include <QLayout>
#include <QWidget>

namespace qs {

void LayoutUtilQS::clear(QLayout* layout, int firstIndex) {
    if (layout == nullptr) return;
    while (layout->count() > firstIndex) {
        QLayoutItem* item = layout->takeAt(firstIndex);
        if (QWidget* w = item->widget()) {
            w->hide();         // 立即不可见，避免在删除前被绘制
            w->deleteLater();  // 回到事件循环后再删除，防止在其信号处理函数中删除自身
        }
        if (QLayout* child = item->layout()) {
            clear(child);      // 递归清理嵌套布局中的控件
        }
        delete item;           // 对子布局而言，这一步同时删除布局对象本身
    }
}

}  // namespace qs
