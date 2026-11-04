#include "FormDialogQS.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace qs {

FormDialogQS::FormDialogQS(QWidget* parent, const QString& title, const QString& subtitle, const QList<FormFieldQS>& fields,
                           const QString& okText, bool danger)
    : QDialog(parent), fields_(fields) {
    setWindowTitle(title);
    setModal(true);
    setMinimumWidth(400);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(26, 22, 26, 20);
    layout->setSpacing(10);
    auto* heading = new QLabel(title, this);
    heading->setObjectName("PanelTitle");
    layout->addWidget(heading);
    if (!subtitle.isEmpty()) {
        auto* sub = new QLabel(subtitle, this);
        sub->setObjectName("SubTitle");
        sub->setWordWrap(true);
        layout->addWidget(sub);
    }
    layout->addSpacing(4);
    for (const FormFieldQS& f : fields_) {
        auto* label = new QLabel(f.label, this);
        label->setStyleSheet("font-weight:600; font-size:12px; color:#4A505A;");
        layout->addWidget(label);
        if (!f.choices.isEmpty()) {
            auto* combo = new QComboBox(this);
            combo->addItems(f.choices);
            combo->setEditable(f.editableChoice);
            if (!f.value.isEmpty()) combo->setCurrentText(f.value);
            layout->addWidget(combo);
            combos_.append(combo);
            edits_.append(nullptr);
        } else {
            auto* edit = new QLineEdit(this);
            edit->setPlaceholderText(f.placeholder);
            edit->setText(f.value);
            if (f.secret) edit->setEchoMode(QLineEdit::Password);
            layout->addWidget(edit);
            edits_.append(edit);
            combos_.append(nullptr);
        }
    }
    layout->addSpacing(8);
    auto* buttons = new QHBoxLayout();
    buttons->addStretch(1);
    auto* cancel = new QPushButton(QString::fromUtf8("取消"), this);
    auto* ok = new QPushButton(okText, this);
    ok->setObjectName(danger ? "DangerButton" : "PrimaryButton");
    ok->setDefault(true);
    buttons->addWidget(cancel);
    buttons->addWidget(ok);
    layout->addLayout(buttons);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(ok, &QPushButton::clicked, this, &QDialog::accept);
    for (QLineEdit* e : edits_) {
        if (e != nullptr) {
            e->setFocus();
            break;
        }
    }
}

QMap<QString, QString> FormDialogQS::values() const {
    QMap<QString, QString> result;
    for (int i = 0; i < fields_.size(); ++i) {
        result[fields_[i].key] = edits_[i] ? edits_[i]->text().trimmed() : combos_[i]->currentText().trimmed();
    }
    return result;
}

bool FormDialogQS::ask(QWidget* parent, const QString& title, const QString& subtitle, const QList<FormFieldQS>& fields,
                       QMap<QString, QString>& out, const QString& okText) {
    FormDialogQS dialog(parent, title, subtitle, fields, okText);
    if (dialog.exec() != QDialog::Accepted) return false;
    out = dialog.values();
    return true;
}

bool FormDialogQS::confirm(QWidget* parent, const QString& title, const QString& text, const QString& okText, bool danger) {
    FormDialogQS dialog(parent, title, text, {}, okText, danger);
    return dialog.exec() == QDialog::Accepted;
}

}  // namespace qs
