#pragma once

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QString>

namespace qs {

/**
 * 矢量图标：用 QPainter 路径绘制，任意缩放都清晰，且不依赖外部图片资源（避免版权问题）。
 * 支持：chat contacts group me home store search plus send info back grid logout heart bell link
 *       check settings close more user-add
 */
class IconQS {
public:
    static QPixmap pixmap(const QString& name, const QColor& color, int size = 22);
    static QIcon icon(const QString& name, const QColor& color, int size = 22);
    /** 普通态与选中态两种颜色。 */
    static QIcon toggleIcon(const QString& name, const QColor& normal, const QColor& checked, int size = 22);
};

}  // namespace qs
