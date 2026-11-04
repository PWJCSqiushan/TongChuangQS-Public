#pragma once

class QLayout;

namespace qs {

/**
 * 布局清理工具。页面刷新时需要清空旧内容再重建；
 * 只删除顶层控件是不够的——放在嵌套布局（addLayout）里的控件会变成"孤儿"留在原位置，
 * 刷新几次后就会出现重叠的旧按钮。这里递归处理所有子布局：先隐藏、再延迟删除。
 */
class LayoutUtilQS {
public:
    /** 清空 layout 中从 firstIndex 起的所有项（含嵌套布局里的控件）。 */
    static void clear(QLayout* layout, int firstIndex = 0);
};

}  // namespace qs
