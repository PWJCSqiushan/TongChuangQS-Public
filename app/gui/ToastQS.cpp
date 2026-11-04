#include "ToastQS.h"

namespace qs {

ToastQS::ToastQS(QWidget* parent) : QLabel(parent) {
    setObjectName("Toast");
    setAlignment(Qt::AlignCenter);
    setWordWrap(true);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    hide();
    timer_.setSingleShot(true);
    QObject::connect(&timer_, &QTimer::timeout, this, &QLabel::hide);
}

void ToastQS::showMessage(const QString& text, bool ok) {
    setText((ok ? QString::fromUtf8("✓  ") : QString::fromUtf8("!  ")) + text);
    setStyleSheet(ok ? "" : "background: rgba(229,72,77,0.95);");
    setMaximumWidth(qMax(260, parentWidget()->width() - 160));
    adjustSize();
    const int w = qMin(width() + 8, maximumWidth());
    resize(w, heightForWidth(w) > 0 ? heightForWidth(w) : height());
    move((parentWidget()->width() - width()) / 2, parentWidget()->height() - height() - 96);
    raise();
    show();
    timer_.start(2600);
}

}  // namespace qs
