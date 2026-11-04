#include "UiAuditQS.h"

#include <QAbstractButton>
#include <QLabel>
#include <QLayout>
#include <QWidget>

namespace qs {

namespace {

bool layoutContains(QLayout* layout, QWidget* w) {
    for (int i = 0; i < layout->count(); ++i) {
        QLayoutItem* item = layout->itemAt(i);
        if (item->widget() == w) return true;
        if (item->layout() && layoutContains(item->layout(), w)) return true;
    }
    return false;
}

QString describe(QWidget* w) {
    QString text;
    if (auto* b = qobject_cast<QAbstractButton*>(w)) text = b->text();
    else if (auto* l = qobject_cast<QLabel*>(w)) text = l->text().left(30);
    const QRect g(w->mapTo(w->window(), QPoint(0, 0)), w->size());
    return QString("%1#%2 \"%3\" @(%4,%5 %6x%7) parent=%8")
        .arg(w->metaObject()->className(), w->objectName(), text)
        .arg(g.x()).arg(g.y()).arg(g.width()).arg(g.height())
        .arg(w->parentWidget()->metaObject()->className());
}

}  // namespace

QStringList UiAuditQS::findOrphans(QWidget* root) {
    QStringList orphans;
    for (QWidget* w : root->findChildren<QWidget*>()) {
        if (w->isWindow() || !w->isVisibleTo(root)) continue;
        if (w->objectName() == "Toast") continue;  // 轻提示按设计手动定位，不在布局中
        QWidget* parent = w->parentWidget();
        if (parent == nullptr) continue;
        if (parent->objectName() == "RowAvatarOverlay" &&
            (w->objectName() == "RowAvatarIcon" || w->objectName() == "RowPresenceDot")) continue;
        if (parent->layout() == nullptr) {
            // Plain application containers have no Qt-owned geometry manager.
            // Leave scroll viewports and native controls to Qt, but inspect their
            // ordinary container counterparts as well as layout-backed parents.
            if (QString::fromLatin1(parent->metaObject()->className()) == "QWidget" &&
                parent->objectName() != "qt_scrollarea_viewport" &&
                (qobject_cast<QAbstractButton*>(w) || qobject_cast<QLabel*>(w))) orphans << describe(w);
            continue;
        }
        if (!layoutContains(parent->layout(), w)) orphans << describe(w);
    }
    return orphans;
}

}  // namespace qs
