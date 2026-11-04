#include "IconQS.h"

#include <QGuiApplication>
#include <QPainter>
#include <QPainterPath>

namespace qs {

namespace {

void draw(QPainter& p, const QString& n) {
    // 在 64×64 的坐标系中绘制
    if (n == "chat") {
        QPainterPath path;
        path.addRoundedRect(QRectF(8, 12, 48, 34), 12, 12);
        p.drawPath(path);
        p.drawLine(QPointF(20, 46), QPointF(16, 56));
        p.drawLine(QPointF(16, 56), QPointF(30, 46));
        p.drawLine(QPointF(22, 25), QPointF(42, 25));
        p.drawLine(QPointF(22, 34), QPointF(36, 34));
    } else if (n == "contacts") {
        p.drawRoundedRect(QRectF(12, 8, 40, 48), 8, 8);
        p.drawEllipse(QPointF(32, 26), 7, 7);
        p.drawArc(QRectF(20, 36, 24, 18), 0, 180 * 16);
        p.drawLine(QPointF(8, 20), QPointF(14, 20));
        p.drawLine(QPointF(8, 32), QPointF(14, 32));
        p.drawLine(QPointF(8, 44), QPointF(14, 44));
    } else if (n == "group") {
        p.drawEllipse(QPointF(24, 22), 8, 8);
        p.drawArc(QRectF(10, 34, 28, 24), 0, 180 * 16);
        p.drawEllipse(QPointF(43, 25), 6.5, 6.5);
        p.drawArc(QRectF(36, 36, 20, 18), 0, 160 * 16);
    } else if (n == "me" || n == "user") {
        p.drawEllipse(QPointF(32, 22), 10, 10);
        p.drawArc(QRectF(14, 36, 36, 30), 0, 180 * 16);
    } else if (n == "user-add") {
        p.drawEllipse(QPointF(26, 22), 9, 9);
        p.drawArc(QRectF(10, 36, 32, 28), 0, 180 * 16);
        p.drawLine(QPointF(48, 20), QPointF(48, 36));
        p.drawLine(QPointF(40, 28), QPointF(56, 28));
    } else if (n == "home") {
        QPainterPath path;
        path.moveTo(10, 30);
        path.lineTo(32, 10);
        path.lineTo(54, 30);
        p.drawPath(path);
        p.drawRoundedRect(QRectF(17, 28, 30, 26), 4, 4);
        p.drawLine(QPointF(32, 42), QPointF(32, 54));
    } else if (n == "store") {
        p.drawRoundedRect(QRectF(12, 28, 40, 26), 4, 4);
        QPainterPath roof;
        roof.moveTo(10, 28);
        roof.lineTo(16, 12);
        roof.lineTo(48, 12);
        roof.lineTo(54, 28);
        p.drawPath(roof);
        p.drawRect(QRectF(26, 38, 12, 16));
    } else if (n == "search") {
        p.drawEllipse(QPointF(28, 28), 14, 14);
        p.drawLine(QPointF(38, 38), QPointF(52, 52));
    } else if (n == "plus") {
        p.drawLine(QPointF(32, 14), QPointF(32, 50));
        p.drawLine(QPointF(14, 32), QPointF(50, 32));
    } else if (n == "send") {
        QPainterPath path;
        path.moveTo(10, 30);
        path.lineTo(54, 12);
        path.lineTo(40, 54);
        path.lineTo(30, 36);
        path.closeSubpath();
        p.drawPath(path);
        p.drawLine(QPointF(30, 36), QPointF(54, 12));
    } else if (n == "info") {
        p.drawEllipse(QPointF(32, 32), 22, 22);
        p.drawLine(QPointF(32, 29), QPointF(32, 44));
        p.drawPoint(QPointF(32, 20));
    } else if (n == "back") {
        p.drawLine(QPointF(40, 14), QPointF(22, 32));
        p.drawLine(QPointF(22, 32), QPointF(40, 50));
    } else if (n == "grid") {
        p.drawRoundedRect(QRectF(12, 12, 16, 16), 4, 4);
        p.drawRoundedRect(QRectF(36, 12, 16, 16), 4, 4);
        p.drawRoundedRect(QRectF(12, 36, 16, 16), 4, 4);
        p.drawRoundedRect(QRectF(36, 36, 16, 16), 4, 4);
    } else if (n == "logout") {
        p.drawLine(QPointF(28, 12), QPointF(14, 12));
        p.drawLine(QPointF(14, 12), QPointF(14, 52));
        p.drawLine(QPointF(14, 52), QPointF(28, 52));
        p.drawLine(QPointF(26, 32), QPointF(52, 32));
        p.drawLine(QPointF(42, 22), QPointF(52, 32));
        p.drawLine(QPointF(52, 32), QPointF(42, 42));
    } else if (n == "heart") {
        QPainterPath path;
        path.moveTo(32, 52);
        path.cubicTo(8, 36, 8, 14, 22, 14);
        path.cubicTo(28, 14, 32, 20, 32, 22);
        path.cubicTo(32, 20, 36, 14, 42, 14);
        path.cubicTo(56, 14, 56, 36, 32, 52);
        p.drawPath(path);
    } else if (n == "bell") {
        QPainterPath path;
        path.moveTo(16, 44);
        path.cubicTo(20, 38, 18, 18, 32, 16);
        path.cubicTo(46, 18, 44, 38, 48, 44);
        path.closeSubpath();
        p.drawPath(path);
        p.drawLine(QPointF(28, 50), QPointF(36, 50));
    } else if (n == "link") {
        p.drawRoundedRect(QRectF(8, 24, 26, 16), 8, 8);
        p.drawRoundedRect(QRectF(30, 24, 26, 16), 8, 8);
    } else if (n == "check") {
        p.drawLine(QPointF(14, 34), QPointF(26, 46));
        p.drawLine(QPointF(26, 46), QPointF(50, 20));
    } else if (n == "close") {
        p.drawLine(QPointF(18, 18), QPointF(46, 46));
        p.drawLine(QPointF(46, 18), QPointF(18, 46));
    } else if (n == "more") {
        p.drawPoint(QPointF(18, 32));
        p.drawPoint(QPointF(32, 32));
        p.drawPoint(QPointF(46, 32));
    } else if (n == "settings") {
        p.drawEllipse(QPointF(32, 32), 9, 9);
        for (int i = 0; i < 8; ++i) {
            p.save();
            p.translate(32, 32);
            p.rotate(i * 45);
            p.drawLine(QPointF(0, -16), QPointF(0, -22));
            p.restore();
        }
        p.drawEllipse(QPointF(32, 32), 17, 17);
    }
}

}  // namespace

QPixmap IconQS::pixmap(const QString& name, const QColor& color, int size) {
    const qreal dpr = qApp ? qApp->devicePixelRatio() : 1.0;
    QPixmap pm(QSize(size, size) * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.scale(size / 64.0, size / 64.0);
    QPen pen(color, (name == "more" || name == "info") ? 6.0 : 4.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    if (name == "more") pen.setWidthF(8.0);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    draw(p, name);
    return pm;
}

QIcon IconQS::icon(const QString& name, const QColor& color, int size) { return QIcon(pixmap(name, color, size)); }

QIcon IconQS::toggleIcon(const QString& name, const QColor& normal, const QColor& checked, int size) {
    QIcon icon;
    icon.addPixmap(pixmap(name, normal, size), QIcon::Normal, QIcon::Off);
    icon.addPixmap(pixmap(name, checked, size), QIcon::Normal, QIcon::On);
    icon.addPixmap(pixmap(name, checked, size), QIcon::Active, QIcon::On);
    return icon;
}

}  // namespace qs
