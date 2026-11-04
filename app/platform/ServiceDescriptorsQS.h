#pragma once

#include <map>
#include <string>

#include "RegistrationFieldQS.h"
#include "ServiceFeatureQS.h"

namespace qs {

/** 注册表单的一个字段。不同服务的注册表单不同，由服务自己描述，界面据此动态生成。 */


/**
 * 服务提供的一项功能（导航栏中的一个入口）。
 * kind 为通用能力（chats/contacts/groups/me）时由平台的通用页面承载，
 * 其他 kind 由服务自己的界面模块提供页面。
 */


/** 表单数据：字段 key → 用户输入。 */
using FormQS = std::map<std::string, std::string>;

}  // namespace qs
