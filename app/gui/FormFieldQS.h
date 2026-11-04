#pragma once

#include <QColor>
#include <QDialog>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

namespace qs {

class FormFieldQS {
public:
    QString key;
    QString label;
    QString placeholder;
    QString value;
    bool secret = false;
    QStringList choices;
    bool editableChoice = false;
};

}  // namespace qs
