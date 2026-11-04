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

namespace qs {

class FeaturePageQS;

class GuiFeatureRegistryQS {
public:
    using FactoryQS = std::function<FeaturePageQS*(ShellContextQS&, QWidget*)>;
    static GuiFeatureRegistryQS& instance() {
        static GuiFeatureRegistryQS registry;
        return registry;
    }
    void add(const std::string& kind, FactoryQS factory) { factories_[kind] = std::move(factory); }
    FeaturePageQS* create(const std::string& kind, ShellContextQS& context, QWidget* parent) const {
        const auto it = factories_.find(kind);
        return it == factories_.end() ? nullptr : it->second(context, parent);
    }

private:
    std::map<std::string, FactoryQS> factories_;
};

}  // namespace qs
