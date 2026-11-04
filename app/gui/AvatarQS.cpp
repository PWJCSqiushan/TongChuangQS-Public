#include "AvatarQS.h"

#include <QGuiApplication>
#include <QPainter>
#include <QPainterPath>

namespace qs {

namespace {

QPixmap canvas(int size) {
    const qreal dpr = qApp ? qApp->devicePixelRatio() : 1.0;
    QPixmap pm(QSize(size, size) * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    return pm;
}

void paintPerson(QPainter& p, const QRectF& r, const QString& seed, const QString& name, bool circle) {
    const QColor bg = AvatarQS::colorFor(seed);
    QPainterPath clip;
    if (circle) clip.addEllipse(r);
    else clip.addRoundedRect(r, r.width() * 0.22, r.width() * 0.22);
    p.fillPath(clip, bg);
    QFont font("Microsoft YaHei UI");
    const QString text = AvatarQS::shortName(name);
    font.setPixelSize(static_cast<int>(r.height() * (text.size() >= 2 ? 0.34 : 0.44)));
    font.setBold(true);
    p.setFont(font);
    p.setPen(Qt::white);
    p.drawText(r, Qt::AlignCenter, text);
}

}  // namespace

QColor AvatarQS::colorFor(const QString& seed) {
    static const char* palette[] = {"#5B8DEF", "#36B37E", "#F59E0B", "#EF6A6A", "#8B5CF6",
                                    "#14B8A6", "#EC6FA9", "#64748B", "#0EA5E9", "#D97706"};
    unsigned hash = 2166136261u;
    for (const QChar c : seed) hash = (hash ^ c.unicode()) * 16777619u;
    return QColor(palette[hash % 10]);
}

QString AvatarQS::shortName(const QString& name) {
    if (name.isEmpty()) return "?";
    if (name.size() <= 2) return name;
    if (name.at(0).unicode() < 128) return name.left(1).toUpper();
    return name.right(2);
}

QPixmap AvatarQS::person(const QString& seed, const QString& name, int size) {
    QPixmap pm = canvas(size);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    paintPerson(p, QRectF(0, 0, size, size), seed, name, true);
    return pm;
}

QPixmap AvatarQS::group(const QStringList& seeds, const QStringList& names, int size) {
    QPixmap pm = canvas(size);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath frame;
    frame.addRoundedRect(QRectF(0, 0, size, size), size * 0.24, size * 0.24);
    p.fillPath(frame, QColor("#E9ECF0"));
    const int n = static_cast<int>(std::min<qsizetype>(4, seeds.size()));
    const qreal gap = size * 0.06;
    const qreal cell = (size - gap * 3) / 2;
    for (int i = 0; i < n; ++i) {
        const qreal x = gap + (i % 2) * (cell + gap);
        const qreal y = (n <= 2 ? (size - cell) / 2 : gap + (i / 2) * (cell + gap));
        paintPerson(p, QRectF(x, y, cell, cell), seeds.at(i), names.at(i).right(1), false);
    }
    return pm;
}

QPixmap AvatarQS::logo(const QColor& color, const QString& text, int size, int radius) {
    QPixmap pm = canvas(size);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    QPainterPath path;
    const qreal r = radius < 0 ? size * 0.28 : radius;
    path.addRoundedRect(QRectF(0, 0, size, size), r, r);
    QLinearGradient g(0, 0, size, size);
    g.setColorAt(0, color.lighter(118));
    g.setColorAt(1, color);
    p.fillPath(path, g);
    QFont font("Microsoft YaHei UI");
    font.setPixelSize(static_cast<int>(size * 0.5));
    font.setBold(true);
    p.setFont(font);
    p.setPen(Qt::white);
    p.drawText(QRectF(0, 0, size, size), Qt::AlignCenter, text);
    return pm;
}

}  // namespace qs
