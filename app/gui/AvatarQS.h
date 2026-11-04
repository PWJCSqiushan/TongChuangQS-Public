#pragma once

#include <QColor>
#include <QPixmap>
#include <QString>
#include <QStringList>

namespace qs {

/**
 * 头像：按账号生成固定的柔和底色 + 昵称文字，圆形；群头像为成员头像的 2×2 拼图（参考常见聊天软件）。
 * 服务标志为圆角方块 + 标志字。全部由程序绘制，不使用外部图片。
 */
class AvatarQS {
public:
    static QPixmap person(const QString& seed, const QString& name, int size);
    static QPixmap group(const QStringList& seeds, const QStringList& names, int size);
    static QPixmap logo(const QColor& color, const QString& text, int size, int radius = -1);
    static QColor colorFor(const QString& seed);
    /** 头像上显示的文字：两字及以下显示全名，否则取后两字。 */
    static QString shortName(const QString& name);
};

}  // namespace qs
