#pragma once

#include "ServiceMessageQS.h"

#include <string>
#include <vector>

namespace qs {

/**
 * 服务总线接口。服务只依赖这个抽象接口，而不依赖平台类本身，
 * 更不依赖其他服务的头文件。PlatformQS 实现该接口负责转发。
 */
class ServiceBusQS {
public:
    virtual ~ServiceBusQS() = default;
    virtual ServiceReplyQS send(const ServiceRequestQS& request) = 0;
    /** 当前平台上运行的全部服务 id（用于广播式查询）。 */
    virtual std::vector<std::string> serviceIds() const = 0;
};

}  // namespace qs
