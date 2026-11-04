#pragma once

#include <QColor>
#include <QString>

class QApplication;

namespace qs {

/**
 * 界面主题：统一的颜色、字体与样式表。
 * 所有服务共用同一套组件样式（风格统一），只有主题色随服务变化（QQ 蓝、微信绿、微博橙、微商紫）。
 */
class ThemeQS {
public:
    static void applyBase(QApplication& app);
    static QString styleSheet(const QColor& accent);

    static QColor mix(const QColor& a, const QColor& b, double t);
    static QColor soft(const QColor& accent) { return mix(accent, QColor("#FFFFFF"), 0.86); }
    static QColor softer(const QColor& accent) { return mix(accent, QColor("#FFFFFF"), 0.93); }
    static QColor dark(const QColor& accent) { return mix(accent, QColor("#000000"), 0.18); }

    static QColor text() { return QColor("#1F2329"); }
    static QColor subtext() { return QColor("#8A8F99"); }
    static QColor border() { return QColor("#E6E8EB"); }
    static QColor canvas() { return QColor("#F3F4F6"); }
    static QColor danger() { return QColor("#E5484D"); }
    static QColor online() { return QColor("#22C55E"); }
};

}  // namespace qs
