#pragma once

#include "ClientQS.h"
#include <QColor>
#include <QString>
#include <QWidget>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include "ShellContextQS.h"
#include "GuiFeatureRegistryQS.h"

namespace qs {

template <class PageQS>
class GuiFeatureAutoRegisterQS {
public:
    explicit GuiFeatureAutoRegisterQS(const char* kind) {
        GuiFeatureRegistryQS::instance().add(kind, [](ShellContextQS& c, QWidget* p) { return new PageQS(c, p); });
    }
};

}  // namespace qs
