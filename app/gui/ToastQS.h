#pragma once

#include <QLabel>
#include <QTimer>

namespace qs {

/** 轻提示：在父窗口底部居中显示一行文字，约 2.6 秒后自动消失。 */
class ToastQS : public QLabel {
public:
    explicit ToastQS(QWidget* parent);
    void showMessage(const QString& text, bool ok = true);

private:
    QTimer timer_;
};

}  // namespace qs
