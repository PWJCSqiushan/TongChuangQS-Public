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

/** 功能页面运行所需的上下文：当前服务、当前账号，以及通知外壳的回调。 */


/**
 * 图形界面中的一个功能页面（抽象基类）。
 * 服务在 features() 中声明有哪些功能，外壳按功能类型从 GuiFeatureRegistryQS 创建页面：
 * 通用能力页面（消息/联系人/群/我）由平台提供，服务特色页面（微博首页、微商小店）由服务自己提供。
 */
class FeaturePageQS : public QWidget {
public:
    FeaturePageQS(ShellContextQS& context, QWidget* parent) : QWidget(parent), context_(context) {}
    ~FeaturePageQS() override = default;

    /** 数据可能已变化（本窗口操作或其他窗口修改），重新读取并刷新显示。 */
    virtual void refresh() = 0;
    /** 聊天事件（新消息、送达回执、上下线）。 */
    virtual void onChatEvents(const std::vector<ChatEventQS>&) {}
    /** 导航栏角标数字。 */
    virtual int badgeCount() const { return 0; }
    /** 外部要求打开某个对象（例如从联系人跳到与某人的会话）。 */
    virtual void activate(const std::string&) {}
    /** 定时刷新在线状态（只更新列表，不重建详情，避免打断正在进行的操作）。 */
    virtual void refreshPresence() {}

protected:
    ShellContextQS& context_;
};


}  // namespace qs

#define QS_REGISTER_GUI_FEATURE(Type, kind) static const ::qs::GuiFeatureAutoRegisterQS<Type> qsGuiFeature##Type(kind);

#include "GuiFeatureRegistryQS.h"
#include "GuiFeatureAutoRegisterQS.h"
