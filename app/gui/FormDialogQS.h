#pragma once

#include <QColor>
#include <QDialog>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

class QLineEdit;
class QComboBox;

#include "FormFieldQS.h"

namespace qs {

/** 表单对话框的一个字段；choices 非空时显示为下拉框。 */


/** 统一风格的表单/确认对话框，代替系统默认的输入框，使界面风格一致。 */
class FormDialogQS : public QDialog {
public:
    FormDialogQS(QWidget* parent, const QString& title, const QString& subtitle, const QList<FormFieldQS>& fields,
                 const QString& okText = QString::fromUtf8("确定"), bool danger = false);
    QMap<QString, QString> values() const;

    /** 弹出表单；用户确认返回 true 并填入 out。 */
    static bool ask(QWidget* parent, const QString& title, const QString& subtitle, const QList<FormFieldQS>& fields,
                    QMap<QString, QString>& out, const QString& okText = QString::fromUtf8("确定"));
    static bool confirm(QWidget* parent, const QString& title, const QString& text,
                        const QString& okText = QString::fromUtf8("确定"), bool danger = false);

private:
    QList<FormFieldQS> fields_;
    QList<QLineEdit*> edits_;
    QList<QComboBox*> combos_;
};

}  // namespace qs
