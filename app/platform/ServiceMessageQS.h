#pragma once

#include "JsonQS.h"

#include <string>

#include "ServiceRequestQS.h"
#include "ServiceReplyQS.h"

namespace qs {

/**
 * 服务间请求。服务之间不能直接调用对方的成员函数（彼此不透明），
 * 只能把请求交给平台的服务总线，由目标服务自己决定如何处理。
 */


/** 服务间应答。 */


}  // namespace qs
